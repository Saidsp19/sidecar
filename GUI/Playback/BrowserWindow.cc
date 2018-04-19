#include "QtCore/QDir"
#include "QtCore/QSettings"
#include "QtGui/QKeyEvent"
#include "QtWidgets/QFileDialog"
#include "QtWidgets/QHeaderView"

#include "GUI/LogUtils.h"
#include "GUI/modeltest.h"

#include "BrowserWindow.h"
#include "MainWindow.h"
#include "RecordingInfo.h"
#include "RecordingModel.h"

using namespace SideCar::GUI::Playback;

static const char* const kBrowseLastIndex = "BrowseLastIndex";
static const char* const kBrowseDirs = "BrowseDirs";
static const char* const kBrowsePath = "BrowsePath";

Logger::Log&
BrowserWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("playback.BrowserWindow");
    return log_;
}

BrowserWindow::BrowserWindow(int shortcut) :
    ToolWindowBase("Browser", "Browser", shortcut), Ui::BrowserWindow(), model_(0)
{
    setupUi(this);
    setWindowFlags(Qt::Window);

    model_ = new RecordingModel(this);
#ifdef __DEBUG__
    new ModelTest(model_, this);
#endif

    recordings_->setModel(model_);
    recordings_->installEventFilter(this);
    recordings_->viewport()->installEventFilter(this);
    recordings_->setAlternatingRowColors(true);
    connect(recordings_, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(handleDoubleClick(const QModelIndex&)));

    QItemSelectionModel* selectionModel = recordings_->selectionModel();
    connect(selectionModel, SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
            SLOT(currentSelectionChanged(const QModelIndex&, const QModelIndex&)));

    QHeaderView* header = recordings_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);

    header = recordings_->verticalHeader();
    header->hide();
    header->setMinimumSectionSize(-1);
    header->setSectionResizeMode(QHeaderView::ResizeToContents);

    QSettings settings;
    int size = settings.beginReadArray(kBrowseDirs);
    for (int index = 0; index < size; ++index) {
        settings.setArrayIndex(index);
        QString path = settings.value(kBrowsePath).toString().trimmed();
        QDir dir(path);
        if (dir.exists()) browseDir_->addItem(path);
    }
    settings.endArray();

    int lastIndex = 0;
    if (size == 0) {
        browseDir_->addItem("/space1/recordings");
        browseDir_->addItem("/space2/recordings");
    } else {
        lastIndex = settings.value(kBrowseLastIndex, 0).toInt();
    }

    browseDir_->setCurrentIndex(lastIndex);
    on_browseDir__activated(lastIndex);
}

void
BrowserWindow::on_browse__clicked()
{
    QString lastPath = browseDir_->currentText();
    QString dirName = "";

    if (lastPath.isEmpty() && browseDir_->count() != 0) {
        lastPath = browseDir_->itemText(0);
    } else {
        QDir dir(lastPath);
        dirName = dir.dirName();
        dir.cdUp();
        lastPath = dir.absolutePath();
    }

    QFileDialog fd(this, "Choose Recordings Directory", lastPath);
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    fd.setFileMode(QFileDialog::DirectoryOnly);
    fd.setReadOnly(true);
    if (!dirName.isEmpty()) fd.selectFile(dirName);

    if (fd.exec()) {
        QString path = fd.selectedFiles()[0];
        if (!path.isEmpty()) browse(path);
    }
}

void
BrowserWindow::on_browseDir__activated(int value)
{
    QString path = browseDir_->itemText(value).trimmed();
    if (path.isEmpty() || !QDir(path).exists()) {
        browseDir_->removeItem(value);
        browseDir_->setCurrentIndex(-1);
    } else {
        browse(path);
        QSettings settings;
        settings.setValue(kBrowseLastIndex, value);
    }
}

bool
BrowserWindow::browse(const QString& path)
{
    Logger::ProcLog log("browse", Log());
    LOGINFO << "path: " << path << std::endl;

    QDir recordingDir(path);

    QStringList nameFilters;
    nameFilters << "*-*";
    recordingDir.setNameFilters(nameFilters);

    QFileInfoList entries = recordingDir.entryInfoList(QDir::Dirs, QDir::Name);
    if (entries.empty()) {
        LOGWARNING << "no entities found" << std::endl;
        LOGTOUT << "false" << std::endl;
        return false;
    }

    model_->clear();
    foreach (QFileInfo fileInfo, entries) {
        LOGDEBUG << "dir: " << fileInfo.filePath() << std::endl;
        QDir dir(fileInfo.filePath());
        model_->add(new RecordingInfo(dir));
    }

    int entry = browseDir_->findText(path);
    if (entry == -1) {
        entry = 0;
        browseDir_->insertItem(0, path);
    }

    browseDir_->setCurrentIndex(entry);

    QSettings settings;
    settings.beginWriteArray(kBrowseDirs, browseDir_->count());
    for (int index = 0; index < browseDir_->count(); ++index) {
        settings.setArrayIndex(index);
        settings.setValue(kBrowsePath, browseDir_->itemText(index));
    }

    adjustColumnSizes();

    LOGTOUT << "true" << std::endl;
    return true;
}

void
BrowserWindow::loadSelected()
{
    QItemSelectionModel* selectionModel = recordings_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    if (!index.isValid()) return;
    handleDoubleClick(index);
}

void
BrowserWindow::handleDoubleClick(const QModelIndex& index)
{
    RecordingInfo* info = model_->GetModelData(index);
    if (info) emit loadRequest(info->getRecordingDirectory());
}

bool
BrowserWindow::eventFilter(QObject* object, QEvent* event)
{
    static Logger::ProcLog log("eventFilter", Log());

    if (object == recordings_) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) { loadSelected(); }
        }
    } else if (object == recordings_->viewport()) {
        // if (event->type() != 12)
        // adjustColumnSizes();
    }
    return Super::eventFilter(object, event);
}

void
BrowserWindow::updateColumns(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    adjustColumnSizes();
}

void
BrowserWindow::adjustColumnSizes()
{
    Logger::ProcLog log("adjustColumnSizes", Log());
    LOGTIN << "begin" << std::endl;

    // Approach: fetch the width of the view port, subtract the column width for all columns but the first one
    // (name), and allocate the remaining space to the first one if it is greater than its minimum width.
    //
    recordings_->resizeColumnsToContents();
    int available(recordings_->viewport()->width());
    for (int column = 0; column < model_->columnCount() - 1; ++column) available -= recordings_->columnWidth(column);

    recordings_->setColumnWidth(RecordingModel::kConfig, available);

    LOGTOUT << std::endl;
}

void
BrowserWindow::resizeEvent(QResizeEvent* event)
{
    adjustColumnSizes();
    Super::resizeEvent(event);
}

void
BrowserWindow::currentSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Logger::ProcLog log("currentSelectionChanged", Log());
    LOGINFO << "current: " << current.row() << " prev: " << previous.row() << std::endl;
    RecordingInfo* info = model_->GetModelData(current);
    if (info) {
        notes_->setPlainText(info->getNotes());
    } else {
        notes_->setPlainText("");
    }
}
