#include <cmath>
#include <time.h>

#include "QtCore/QSettings"
#include "QtGui/QCloseEvent"
#include "QtGui/QFileDialog"
#include "QtGui/QHeaderView"
#include "QtGui/QInputDialog"
#include "QtGui/QMessageBox"
#include "QtGui/QPainter"
#include "QtGui/QStatusBar"
#include "QtGui/QStyledItemDelegate"

#include "GUI/LogUtils.h"
#include "GUI/modeltest.h"

#include "App.h"
#include "BrowserWindow.h"
#include "Clock.h"
#include "Emitter.h"
#include "FileModel.h"
#include "MainWindow.h"
#include "NotesWindow.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::Playback;

static const char* const kRecordingDirs = "RecordingDirs";
static const char* const kRecordingPath = "RecordingPath";
static const char* const kAddress = "Address";
static const char* const kSuffix = "Suffix";
static const char* const kRateMultiple = "RateMultiple";

static const char* const kRegionLoop = "RegionLoop";
static const char* const kRegionStart = "RegionStart";
static const char* const kRegionStartIndex = "RegionStartIndex";
static const char* const kRegionEnd = "RegionEnd";
static const char* const kRegionEndIndex = "RegionEndIndex";
static const char* const kBookmarks = "Bookmarks";
static const char* const kBookmarkName = "Name";
static const char* const kBookmarkWhen = "When";
static const char* const kBookmarkToolTip = "ToolTip";

/** Variant of QStyledItemDelegate used to show loading progress for an Emitter. Installed in the files_
    QTableView for the FileModel::kName column. Draws a progress bar in the background that shows how much of
    the PRI file the Emitter object has processed during its load.
*/
struct NameItemDelegate : public QStyledItemDelegate {
    using Super = QStyledItemDelegate;

    /** Constructor

        \param parent owner of this object
    */
    NameItemDelegate(QObject* parent = 0) : Super(parent) {}

    /** Override of QStyledItemDelegate::paint method. If the Emitter object being updated has a non-zero load
        percentage, paint part of the background rectangle a different color to illustrate the percentage
        loaded.

        \param painter QPainter object to use for painting

        \param option display settings to use

        \param index location of the item in the model
    */
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

void
NameItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const FileModel* model = static_cast<const FileModel*>(index.model());
    const Emitter* emitter = model->getEmitter(index.row());
    double loadPercentage = emitter->getLoadPercentage();
    if (loadPercentage) {
        QRect rect = option.rect;
        rect.setWidth(rect.width() * loadPercentage);
        painter->fillRect(rect, option.palette.highlight());
    }
    Super::paint(painter, option, index);
}

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("playback.MainWindow");
    return log_;
}

MainWindow::MainWindow() :
    MainWindowBase(), Ui::MainWindow(), clock_(0), model_(0), notesWindow_(0), wallClockRatePower_(0)
{
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
#ifdef __DEBUG__
    setWindowTitle("Playback (DEBUG)");
#endif

    startTime_->setText("--:--:--");
    endTime_->setText("--:--:--");
    duration_->setText("--:--:--");

    BrowserWindow* browser = App::GetApp()->getBrowserWindow();
    connect(browser, SIGNAL(loadRequest(const QString&)), SLOT(load(const QString&)));

    QAction* action = browser->getShowHideAction();
    action->setIcon(QIcon(":/browser.svg"));
    showBrowser_->setDefaultAction(action);

    recordingInfo_->setEnabled(false);
    playbackControl_->setEnabled(false);
    bookmarksControl_->setEnabled(false);
    actionStart_->setEnabled(false);
    actionRewind_->setEnabled(false);

    for (int index = 0; index < kMaxRecentFiles; ++index) {
        QAction* action = recentFiles_[index] = new QAction(this);
        action->setShortcut(QKeySequence(QString("CTRL+%1").arg(index + 1)));
        connect(action, SIGNAL(triggered()), SLOT(openRecentDir()));
        menuRecentFiles_->addAction(action);
        action->setVisible(false);
    }

    QSettings settings;
    int size = settings.beginReadArray(kRecordingDirs);
    for (int index = 0; index < size; ++index) {
        settings.setArrayIndex(index);
        QString path = settings.value(kRecordingPath).toString().trimmed();
        QDir dir(path);
        if (dir.exists()) {
            recordingDir_->addItem(path);
            if (index < kMaxRecentFiles) {
                menuRecentFiles_->setEnabled(true);
                QString text(QString("&%1 %2").arg(index + 1).arg(path));
                recentFiles_[index]->setText(text);
                recentFiles_[index]->setData(path);
                recentFiles_[index]->setVisible(true);
            }
        }
    }
    settings.endArray();

    if (size == 0) recordingDir_->addItem("/space1/recordings/last");

    recordingDir_->setInsertPolicy(QComboBox::InsertAtTop);
    recordingDir_->setCurrentIndex(-1);

    address_->setText(settings.value(kAddress, "237.1.2.150").toString());
    suffix_->setText(settings.value(kSuffix, "").toString());

    clock_ = new Clock(this, 100, getWallClockRate());

    connect(clock_, SIGNAL(tick(const Time::TimeStamp&, const Time::TimeStamp&)),
            SLOT(clockTick(const Time::TimeStamp&, const Time::TimeStamp&)));
    connect(clock_, SIGNAL(started()), SLOT(clockStarted()));
    connect(clock_, SIGNAL(stopped()), SLOT(clockStopped()));
    connect(clock_, SIGNAL(playbackClockStartChanged(const Time::TimeStamp&)),
            SLOT(playbackClockStartChanged(const Time::TimeStamp&)));

    rateMultiplier_->setCurrentIndex(settings.value(kRateMultiple, 3).toInt());
    regionLoop_->setCurrentIndex(settings.value(kRegionLoop, 1).toInt());

    model_ = new FileModel(this);
#ifdef __DEBUG__
    new ModelTest(model_, this);
#endif

    connect(model_, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            SLOT(updateColumns(const QModelIndex&, const QModelIndex&)));
    connect(model_, SIGNAL(loadComplete()), SLOT(loaded()));

    files_->setModel(model_);
    files_->installEventFilter(this);
    files_->viewport()->installEventFilter(this);
    files_->setItemDelegateForColumn(FileModel::kName, new NameItemDelegate(this));

    QHeaderView* header = files_->horizontalHeader();
    header->setResizeMode(QHeaderView::Fixed);

    header = files_->verticalHeader();
    header->hide();
    header->setMinimumSectionSize(-1);
    header->setResizeMode(QHeaderView::ResizeToContents);

    updateRecentFileActions();
}

void
MainWindow::openRecentDir()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString path(action->data().toString());
        load(path);
    }
}

void
MainWindow::on_actionLoad__triggered()
{
    on_load__clicked();
}

void
MainWindow::on_actionStart__triggered()
{
    on_startStop__clicked();
}

void
MainWindow::on_actionRewind__triggered()
{
    on_rewind__clicked();
}

void
MainWindow::on_load__clicked()
{
    QString lastPath = recordingDir_->currentText();
    QString dirName = "";

    if (lastPath.isEmpty() && recordingDir_->count() != 0) {
        lastPath = recordingDir_->itemText(0);
    } else {
        QDir dir(lastPath);
        dirName = dir.dirName();
        dir.cdUp();
        lastPath = dir.absolutePath();
    }

    QFileDialog fd(this, "Choose Recording Directory", lastPath);
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    fd.setFileMode(QFileDialog::DirectoryOnly);
    fd.setReadOnly(true);
    if (!dirName.isEmpty()) fd.selectFile(dirName);

    if (fd.exec()) {
        QString path = fd.selectedFiles()[0];
        if (!path.isEmpty()) {
            load(path);
            return;
        }
    }

    statusBar()->showMessage("Load canceled", 5000);
}

void
MainWindow::on_recordingDir__activated(int value)
{
    QString path = recordingDir_->itemText(value).trimmed();
    if (path.isEmpty() || !QDir(path).exists()) {
        statusBar()->showMessage("Invalid directory");
        recordingDir_->removeItem(value);
        recordingDir_->setCurrentIndex(-1);
        return;
    }

    load(path);
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    Logger::ProcLog log("closeEvent", Log());
    LOGINFO << "load_->isEnabled: " << load_->isEnabled() << std::endl;
    if (load_->isEnabled()) {
        event->accept();
        Super::closeEvent(event);
    } else {
        event->ignore();
    }
}

void
MainWindow::load(const QString& path)
{
    BrowserWindow* browser = App::GetApp()->getBrowserWindow();
    browser->setEnabled(false);
    load_->setEnabled(false);
    recordingDir_->setEnabled(false);

    delete notesWindow_;
    notesWindow_ = 0;

    raise();

    pendingPath_ = path;
    clock_->stop();

    startTime_->setText("--:--:--");
    endTime_->setText("--:--:--");
    duration_->setText("--:--:--");
    regionStart_->clear();
    regionStart_->clearEditText();
    regionEnd_->clear();
    regionEnd_->clearEditText();
    bookmarks_->clear();

    recordingInfo_->setEnabled(false);
    playbackControl_->setEnabled(false);
    bookmarksControl_->setEnabled(false);
    actionStart_->setEnabled(false);
    actionRewind_->setEnabled(false);

    notes_->setEnabled(false);
    clockAbsolute_->setText("--:--:--");
    clockElapsed_->setText("");

    model_->beginLoad(path);

    adjustColumnSizes();
}

void
MainWindow::loaded()
{
    BrowserWindow* browser = App::GetApp()->getBrowserWindow();
    browser->setEnabled(true);
    load_->setEnabled(true);
    recordingDir_->setEnabled(true);

    activePath_ = pendingPath_;
    recordingInfo_->setEnabled(true);
    playbackControl_->setEnabled(true);
    bookmarksControl_->setEnabled(true);
    bookmarkDelete_->setEnabled(false);
    actionStart_->setEnabled(true);
    actionRewind_->setEnabled(true);

    // Fetch the youngest and oldest times found in the recordings.a
    //
    Time::TimeStamp start(model_->getStartTime());
    Time::TimeStamp end(model_->getEndTime());
    Time::TimeStamp elapsed(end);
    elapsed -= start;
    regionStartTime_ = start;
    regionEndTime_ = end;

    long secs = start.getSeconds();
    long hours = secs / 3600;
    secs -= hours * 3600;
    long mins = secs / 60;
    secs -= mins * 60;
    hours = hours % 24;
    zero_ = start;
    zero_ -= Time::TimeStamp(hours * 3600 + mins * 60 + secs, 0);

    // Finally, set the labels with the formatted times.
    //
    startTime_->setText(clock_->formatTimeStamp(start));
    endTime_->setText(clock_->formatTimeStamp(end));
    duration_->setText(clock_->formatDuration(elapsed));

    QDir dir(activePath_);
    notes_->setEnabled(dir.exists("notes.txt"));

    if (dir.exists("bookmarks.ini")) {
        QSettings settings(dir.filePath("bookmarks.ini"), QSettings::IniFormat);

        int count = settings.beginReadArray(kBookmarks);
        for (int index = 0; index < count; ++index) {
            settings.setArrayIndex(index);
            QString name = settings.value(kBookmarkName).toString();
            double when = settings.value(kBookmarkWhen).toDouble();
            QString toolTip = settings.value(kBookmarkToolTip).toString();
            bookmarks_->addItem(name, when);
            bookmarks_->setItemData(index, toolTip, Qt::ToolTipRole);
        }
        settings.endArray();
        bookmarkDelete_->setEnabled(bookmarks_->count() != 0);

        regionStart_->clear();
        count = settings.beginReadArray(kRegionStart);
        for (int index = 0; index < count; ++index) {
            settings.setArrayIndex(index);
            QString name = settings.value(kBookmarkName).toString();
            double when = settings.value(kBookmarkWhen).toDouble();
            QString toolTip = settings.value(kBookmarkToolTip).toString();
            regionStart_->addItem(name, when);
            regionStart_->setItemData(index, toolTip, Qt::ToolTipRole);
        }
        settings.endArray();
        regionStart_->setCurrentIndex(settings.value(kRegionStartIndex, 0).toInt());
        regionStart_->setToolTip(regionStart_->itemData(regionStart_->currentIndex(), Qt::ToolTipRole).toString());

        regionEnd_->clear();
        count = settings.beginReadArray(kRegionEnd);
        for (int index = 0; index < count; ++index) {
            settings.setArrayIndex(index);
            QString name = settings.value(kBookmarkName).toString();
            double when = settings.value(kBookmarkWhen).toDouble();
            QString toolTip = settings.value(kBookmarkToolTip).toString();
            regionEnd_->addItem(name, when);
            regionEnd_->setItemData(index, toolTip, Qt::ToolTipRole);
        }
        settings.endArray();
        regionEnd_->setCurrentIndex(settings.value(kRegionEndIndex, 0).toInt());
        regionEnd_->setToolTip(regionEnd_->itemData(regionEnd_->currentIndex(), Qt::ToolTipRole).toString());

        regionStartTime_ = regionStart_->itemData(regionStart_->currentIndex()).toDouble();
        regionEndTime_ = regionEnd_->itemData(regionEnd_->currentIndex()).toDouble();
    } else {
        QString nowText = QString("%1 %2").arg(startTime_->text()).arg(clock_->formatDuration(0.0));
        regionStart_->addItem("Recording Start", start.asDouble());
        regionStart_->setItemData(0, nowText, Qt::ToolTipRole);
        regionStart_->setCurrentIndex(0);
        regionStart_->setToolTip(nowText);
        nowText = QString("%1 %2").arg(endTime_->text()).arg(duration_->text());
        regionEnd_->addItem("Recording End", end.asDouble());
        regionEnd_->setItemData(0, nowText, Qt::ToolTipRole);
        regionEnd_->setCurrentIndex(0);
        regionEnd_->setToolTip(nowText);
    }

    clock_->setClockRange(regionStartTime_, regionEndTime_);
    statusBar()->showMessage(QString("Loaded %1").arg(activePath_), 5000);

    int entry = recordingDir_->findText(activePath_);
    if (entry == -1) {
        entry = 0;
        recordingDir_->insertItem(0, activePath_);
        updateRecentFileActions();
    }

    recordingDir_->setCurrentIndex(entry);

    QSettings settings;
    settings.beginWriteArray(kRecordingDirs, recordingDir_->count());
    for (int index = 0; index < kMaxRecentFiles; ++index) {
        settings.setArrayIndex(index);
        settings.setValue(kRecordingPath, recordingDir_->itemText(index));
    }

    setWindowTitle(QString("Playback - ") + dir.dirName());
#ifdef __DEBUG__
    setWindowTitle(QString("Playback (DEBUG) - ") + dir.dirName());
#endif

    adjustColumnSizes();

    startStop_->setFocus();
}

void
MainWindow::updateRecentFileActions()
{
    for (int index = 0; index < recordingDir_->count() && index < kMaxRecentFiles; ++index) {
        QString text(QString("&%1 %2").arg(index + 1).arg(recordingDir_->itemText(index)));
        recentFiles_[index]->setText(text);
        recentFiles_[index]->setData(recordingDir_->itemText(index));
        recentFiles_[index]->setVisible(true);
    }

    for (int index = recordingDir_->count(); index < kMaxRecentFiles; ++index) recentFiles_[index]->setVisible(false);

    menuRecentFiles_->setEnabled(recordingDir_->count() > 0);
}

void
MainWindow::on_notes__clicked()
{
    if (!notesWindow_) notesWindow_ = new NotesWindow(recordingDir_->currentText());
    notesWindow_->show();
    notesWindow_->raise();
    notesWindow_->activateWindow();
    statusBar()->showMessage("Opened notes", 5000);
}

void
MainWindow::on_address__editingFinished()
{
    QSettings settings;
    if (settings.value(kAddress).toString() != address_->text()) {
        settings.setValue(kAddress, address_->text());
        emit addressChanged(address_->text());
        statusBar()->showMessage("Changed multicast address", 5000);
    }
}

void
MainWindow::on_suffix__editingFinished()
{
    QSettings settings;
    if (settings.value(kSuffix).toString() != suffix_->text()) {
        settings.setValue(kSuffix, suffix_->text());
        emit suffixChanged(suffix_->text());
        statusBar()->showMessage("Changed publisher suffix", 5000);
    }
}

void
MainWindow::on_startStop__clicked()
{
    if (startStop_->text() == "Start") {
        clock_->start();
    } else {
        clock_->stop();
    }
}

void
MainWindow::clockStarted()
{
    address_->setEnabled(false);
    suffix_->setEnabled(false);
    startStop_->setText("Stop");
    startStop_->setToolTip("Click to stop playback");
    actionStart_->setText("Stop");
    statusBar()->showMessage("Started", 5000);
}

void
MainWindow::clockStopped()
{
    address_->setEnabled(true);
    suffix_->setEnabled(true);
    startStop_->setText("Start");
    startStop_->setToolTip("Click to start playback");
    actionStart_->setText("Start");
    statusBar()->showMessage("Stopped", 5000);
}

void
MainWindow::on_rewind__clicked()
{
    clock_->setPlaybackClockStart(regionStartTime_);
    statusBar()->showMessage("Rewound to 'Start' time.", 5000);
}

void
MainWindow::on_regionLoop__currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(kRegionLoop, index);
    if (index == 1)
        statusBar()->showMessage("Looping at 'End' time", 5000);
    else
        statusBar()->showMessage("Stopping at 'End' time", 5000);
}

Time::TimeStamp
MainWindow::createTimeStamp(QString spec) const
{
    Logger::ProcLog log("createTimeStamp", Log());
    LOGINFO << "spec: " << spec << std::endl;

    bool isOffset = false;
    bool ok = false;
    double value;

    if (spec[0] == '+' || spec[0] == '-') {
        // Relative time given in floating-point hours, minutes or seconds, depending on optional suffix
        // character.
        //
        double factor = 1.0;
        char c = spec.at(spec.length() - 1).toLatin1();
        if (c == 'h' || c == 'H')
            factor = 3600.0;
        else if (c == 'm' || c == 'M')
            factor = 60.0;
        if (factor != 1.0 || c == 's' || c == 'S') spec.remove(spec.length() - 1, 1);
        isOffset = true;
        value = spec.toDouble(&ok) * factor;
        LOGDEBUG << "offset value: " << value << std::endl;
    } else {
        // Absolute time given in HH:MM:SS.sss or floating-point seconds since midnight (0-86400)
        //
        if (spec.length() > 7 && (spec[2] == '.' || spec[2] == ':') && (spec[5] == '.' || spec[5] == ':')) {
            int hrs = spec[0].digitValue() * 10 + spec[1].digitValue();
            int min = spec[3].digitValue() * 10 + spec[4].digitValue();
            spec.remove(0, 6);

            double secs = spec.toDouble(&ok);
            if (!ok) {
                secs = spec.toInt(&ok);
                if (!ok) {
                    secs = 0.0;
                    ok = true;
                }
            }
            value = hrs * 3600.0 + min * 60.0 + secs;
        } else {
            value = spec.toDouble(&ok);
            if (!ok) value = spec.toInt(&ok);
        }

        LOGDEBUG << "absolute value: " << value << std::endl;
    }

    if (!ok) return Time::TimeStamp::Min();

    Time::TimeStamp when;
    if (isOffset) {
        when = clock_->getPlaybackClock();
        when += value;
    } else {
        when = zero_;
        when += value;
    }

    LOGDEBUG << "when: " << when << std::endl;
    return when;
}

void
MainWindow::on_jumpTo__activated(int index)
{
    Logger::ProcLog log("on_jumpTo__activated", Log());
    LOGINFO << "index: " << index << std::endl;
    if (index == -1) return;

    QString text = jumpTo_->currentText().trimmed();
    LOGDEBUG << "text: " << text << std::endl;
    jumpTo_->setCurrentIndex(-1);

    Time::TimeStamp when = createTimeStamp(text);
    if (when == Time::TimeStamp::Min()) {
        QMessageBox::information(this, "Invalid Jump", "Invalid jump specification.", QMessageBox::Ok);
        return;
    }

    clock_->setPlaybackClockStart(when);

    statusBar()->showMessage(QString("Jumped to %1").arg(clock_->formatTimeStamp(when)), 5000);
}

void
MainWindow::on_regionStart__activated(int index)
{
    Logger::ProcLog log("on_regionStart__activated", Log());
    LOGINFO << "index: " << index << std::endl;
    if (index == -1) return;

    QVariant tmp = regionStart_->itemData(index);
    if (!tmp.isValid()) {
        QString text = regionStart_->currentText().split(' ')[0];
        LOGDEBUG << "text: " << text << std::endl;
        Time::TimeStamp when = createTimeStamp(text);
        if (when == Time::TimeStamp::Min() || when < model_->getStartTime() || when > model_->getEndTime()) {
            QMessageBox::information(this, "Invalid Start Time", "Invalid start time specification.", QMessageBox::Ok);
            regionStart_->setCurrentIndex(-1);
            regionStart_->removeItem(index);
            return;
        }

        tmp = when.asDouble();
        regionStart_->setItemData(index, tmp);
        regionStart_->setItemData(index, text, Qt::ToolTipRole);
    }

    regionStartTime_ = tmp.toDouble();
    clock_->setClockRange(regionStartTime_, regionEndTime_);
    regionStart_->setToolTip(regionStart_->itemData(index, Qt::ToolTipRole).toString());

    writeBookmarks();

    statusBar()->showMessage(QString("Set start time to %1").arg(clock_->formatTimeStamp(regionStartTime_)), 5000);
}

void
MainWindow::on_regionEnd__activated(int index)
{
    Logger::ProcLog log("on_regionEnd__activated", Log());
    LOGINFO << "index: " << index << std::endl;
    if (index == -1) return;

    QVariant tmp = regionEnd_->itemData(index);
    if (!tmp.isValid()) {
        QString text = regionEnd_->currentText().split(' ')[0];
        LOGDEBUG << "text: " << text << std::endl;
        Time::TimeStamp when = createTimeStamp(text);
        if (when == Time::TimeStamp::Min() || when < model_->getStartTime() || when > model_->getEndTime()) {
            QMessageBox::information(this, "Invalid End Time", "Invalid end time specification.", QMessageBox::Ok);
            regionEnd_->setCurrentIndex(-1);
            regionEnd_->removeItem(index);
            return;
        }

        tmp = when.asDouble();
        regionEnd_->setItemData(index, tmp);
        regionEnd_->setItemData(index, text, Qt::ToolTipRole);
    }

    regionEndTime_ = tmp.toDouble();
    clock_->setClockRange(regionStartTime_, regionEndTime_);
    regionEnd_->setToolTip(regionEnd_->itemData(index, Qt::ToolTipRole).toString());

    writeBookmarks();

    statusBar()->showMessage(QString("Set end time to %1").arg(clock_->formatTimeStamp(regionEndTime_)), 5000);
}

void
MainWindow::on_bookmarks__activated(int index)
{
    Logger::ProcLog log("on_bookmarks__activated", Log());
    LOGINFO << "index: " << index << std::endl;
    if (index == -1) return;
    Time::TimeStamp when(bookmarks_->itemData(index).toDouble());
    LOGDEBUG << "when: " << when << std::endl;
    clock_->setPlaybackClockStart(when);
}

void
MainWindow::on_bookmarkAdd__clicked()
{
    Logger::ProcLog log("on_bookmarkAdd__clicked", Log());
    Time::TimeStamp now = clock_->getPlaybackClock().asDouble();
    Time::TimeStamp duration = now;
    duration -= model_->getStartTime();

    QString nowText = QString("%1 %2").arg(clock_->formatTimeStamp(now)).arg(clock_->formatDuration(duration));

    while (1) {
        QString name = QInputDialog::getText(this, "New Bookmark", QString("Name for time %1:").arg(nowText));

        if (name.isEmpty()) return;

        for (int index = 0; index < bookmarks_->count(); ++index) {
            if (bookmarks_->itemText(index).split(' ')[0] == name) {
                QMessageBox::information(this, "Invalid Bookmark Name", "Bookmark name already exists.",
                                         QMessageBox::Ok);
                name = "";
                break;
            }
        }

        if (!name.isEmpty()) {
            double when = now.asDouble();
            int index = bookmarks_->count();
            bookmarks_->addItem(name, when);
            bookmarks_->setItemData(index, nowText, Qt::ToolTipRole);
            index = regionStart_->count();
            regionStart_->addItem(name, when);
            regionStart_->setItemData(index, nowText, Qt::ToolTipRole);
            index = regionEnd_->count();
            regionEnd_->addItem(name, when);
            regionEnd_->setItemData(index, nowText, Qt::ToolTipRole);
            writeBookmarks();
            statusBar()->showMessage("Added bookmark at " + nowText);
            break;
        }
    }
}

void
MainWindow::on_bookmarkDelete__clicked()
{
    Logger::ProcLog log("on_bookmarkDelete__clicked", Log());
    int index = bookmarks_->currentIndex();
    if (index == -1) return;
    QString name = bookmarks_->itemText(index);
    bookmarks_->removeItem(index);
    bookmarks_->setCurrentIndex(-1);
    for (index = 0; index < regionStart_->count(); ++index) {
        if (regionStart_->itemText(index) == name) {
            if (index == regionStart_->currentIndex()) regionStart_->setCurrentIndex(-1);
            regionStart_->removeItem(index);
            break;
        }
    }

    for (index = 0; index < regionEnd_->count(); ++index) {
        if (regionEnd_->itemText(index) == name) {
            if (index == regionEnd_->currentIndex()) regionEnd_->setCurrentIndex(-1);
            regionEnd_->removeItem(index);
            break;
        }
    }

    writeBookmarks();
    statusBar()->showMessage(QString("Deleted bookmark '%1'").arg(name), 5000);
}

void
MainWindow::writeBookmarks()
{
    QDir dir(activePath_);
    QSettings settings(dir.filePath("bookmarks.ini"), QSettings::IniFormat);

    int count = bookmarks_->count();
    settings.beginWriteArray(kBookmarks, count);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        settings.setValue(kBookmarkName, bookmarks_->itemText(index));
        settings.setValue(kBookmarkWhen, bookmarks_->itemData(index));
        settings.setValue(kBookmarkToolTip, bookmarks_->itemData(index, Qt::ToolTipRole));
    }
    settings.endArray();

    count = regionStart_->count();
    settings.beginWriteArray(kRegionStart, count);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        settings.setValue(kBookmarkName, regionStart_->itemText(index));
        settings.setValue(kBookmarkWhen, regionStart_->itemData(index));
        settings.setValue(kBookmarkToolTip, regionStart_->itemData(index, Qt::ToolTipRole));
    }
    settings.endArray();
    settings.setValue(kRegionStartIndex, regionStart_->currentIndex());

    count = regionEnd_->count();
    settings.beginWriteArray(kRegionEnd, count);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        settings.setValue(kBookmarkName, regionEnd_->itemText(index));
        settings.setValue(kBookmarkWhen, regionEnd_->itemData(index));
        settings.setValue(kBookmarkToolTip, regionEnd_->itemData(index, Qt::ToolTipRole));
    }
    settings.endArray();
    settings.setValue(kRegionEndIndex, regionEnd_->currentIndex());

    bookmarkDelete_->setEnabled(bookmarks_->count() != 0);
}

double
MainWindow::getWallClockRate() const
{
    return std::pow(2.0, wallClockRatePower_);
}

void
MainWindow::clockTick(const Time::TimeStamp& playbackClock, const Time::TimeStamp& elapsed)
{
    clockAbsolute_->setText(clock_->formatTimeStamp(playbackClock));
    clockElapsed_->setText(clock_->formatDuration(elapsed));
    if (playbackClock > regionEndTime_) {
        clock_->stop();
        if (regionLoop_->currentIndex() == 1) {
            clock_->setPlaybackClockStart(regionStartTime_);
            clock_->start();
        }
    }
}

void
MainWindow::playbackClockStartChanged(const Time::TimeStamp& playbackClock)
{
    clockAbsolute_->setText(clock_->formatTimeStamp(playbackClock));
    clockElapsed_->setText(clock_->formatDuration(0));
}

void
MainWindow::on_rateMultiplier__currentIndexChanged(int index)
{
    QSettings settings;
    settings.setValue(kRateMultiple, index);

    wallClockRatePower_ = 3 - index;
    double rate = getWallClockRate();
    if (clock_) clock_->setWallClockRate(rate);

    QString label("Rate changed to ");
    if (wallClockRatePower_ < 0) {
        label += QString("x 1/%1").arg(std::pow(2.0, -wallClockRatePower_));
    } else {
        label += QString("x %1").arg(rate);
    }

    statusBar()->showMessage(label, 5000);
}

void
MainWindow::updateColumns(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    adjustColumnSizes();
}

void
MainWindow::adjustColumnSizes()
{
    // Approach: fetch the width of the view port, subtract the column width for all columns but the first one
    // (name), and allocate the remaining space to the first one if it is greater than its minimum width.
    //
    files_->resizeColumnsToContents();
    int minimum = files_->columnWidth(0);
    int available(files_->viewport()->width());
    for (int column = 0; column < model_->columnCount(); ++column)
        if (column != FileModel::kName) available -= files_->columnWidth(column);
    if (available >= minimum) files_->setColumnWidth(FileModel::kName, available);
}

bool
MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if (object == files_ || object == files_->viewport()) {
        if (event->type() == QEvent::Resize) { adjustColumnSizes(); }
    }
    return Super::eventFilter(object, event);
}
