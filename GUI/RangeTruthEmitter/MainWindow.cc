#include <sys/time.h>

#include <cmath>

#include "ace/Message_Block.h"

#include "QtCore/QSettings"
#include "QtCore/QString"
#include "QtCore/QTextStream"
#include "QtCore/QTimer"
#include "QtGui/QCloseEvent"
#include "QtWidgets/QFileDialog"
#include "QtWidgets/QHeaderView"
#include "QtWidgets/QMessageBox"
#include "QtWidgets/QStatusBar"

#include "GUI/LogUtils.h"
#include "GUI/Utils.h"
#include "GUI/Writers.h"
#include "GeoStars/geoStars.h"
#include "IO/MessageManager.h"
#include "IO/Writers.h"
#include "Messages/RadarConfig.h"
#include "Time/TimeStamp.h"
#include "Utils/Utils.h"

#include "AccelerationEditor.h"
#include "AddressValidator.h"
#include "DurationEditor.h"
#include "MainWindow.h"
#include "SegmentModel.h"
#include "VelocityEditor.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::RangeTruthEmitter;

Logger::Log&
MainWindow::Log()
{
    Logger::Log& log_ = Logger::Log::Find("RangeTruthEmitter.MainWindow");
    return log_;
}

MainWindow::MainWindow() :
    MainWindowBase(), Ui::MainWindow(), writer_(0), timer_(new QTimer(this)), model_(new SegmentModel(this)),
    elapsedTime_(0.0), activeSegment_(-1), xyz_(3, 0.0), lastFile_(), tcnMsg_(new ACE_Message_Block(20)),
    reportCounter_(0)
{
    Logger::ProcLog log("MainWindow", Log());

    setupUi(this);

    connect(model_, SIGNAL(segmentChanged()), SLOT(flagDirtyWindow()));

    segments_->setModel(model_);

#if 0
  VelocityEditor* ve = new VelocityEditor(segments_);
  segments_->setItemDelegateForColumn(SegmentModel::kXVel, ve);
  segments_->setItemDelegateForColumn(SegmentModel::kYVel, ve);
  segments_->setItemDelegateForColumn(SegmentModel::kZVel, ve);

  AccelerationEditor* ae = new AccelerationEditor(segments_);
  segments_->setItemDelegateForColumn(SegmentModel::kXAcc, ae);
  segments_->setItemDelegateForColumn(SegmentModel::kYAcc, ae);
  segments_->setItemDelegateForColumn(SegmentModel::kZAcc, ae);

  DurationEditor* de = new DurationEditor(segments_);
  segments_->setItemDelegateForColumn(SegmentModel::kDuration, de);
#endif

    QHeaderView* header = segments_->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);

    getApp()->setVisibleWindowMenuNew(false);
    getApp()->setVisibleWindowMenuMaximize(false);
    getApp()->setVisibleWindowMenuFullScreen(false);

    new AddressValidator(address_);

    QSettings settings;
    lastAddress_ = "237.1.2.3";
    address_->setText(settings.value("Address", lastAddress_).toString());
    if (address_->text().isEmpty())
        address_->setText(lastAddress_);
    else
        lastAddress_ = address_->text();

    // address_->selectAll();
    port_->setValue(settings.value("Port", 5123).toInt());
    systemId_->setValue(settings.value("SystemId", 1).toInt());
    rate_->setValue(settings.value("Rate", 1.0).toDouble());
    rangeFormat_->setChecked(settings.value("RangeFormat", false).toBool());

    manualRange_->setValue(settings.value("Range", 0.0).toDouble());
    manualAzimuth_->setValue(settings.value("Azimuth", 0.0).toDouble());
    manualElevation_->setValue(settings.value("Elevation", 0.0).toDouble());

    updateTargetPositionRAE(false);

    connect(timer_, SIGNAL(timeout()), SLOT(simulatorTimeout()));
    setTimerInterval(rate_->value());

    QItemSelectionModel* selectionModel = segments_->selectionModel();
    connect(selectionModel, SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), SLOT(updateButtons()));

    for (int index = 0; index < kMaxRecentFiles; ++index) {
        QAction* action = recentFiles_[index] = new QAction(this);
        action->setShortcut(QKeySequence(QString("CTRL+%1").arg(index + 1)));
        connect(action, SIGNAL(triggered()), SLOT(openRecentFile()));
        menuRecentFiles_->addAction(action);
    }

    menuRecentFiles_->addSeparator();
    menuRecentFiles_->addAction(actionClear_);

    updateRecentFileActions();
    updateButtons();

    setWindowTitle("RangeTruthEmitter - Untitled[*]");
    makeWriter();

    statusBar()->showMessage("Ready", 5000);
}

MainWindow::~MainWindow()
{
    removeWriter();
    tcnMsg_->release();
}

void
MainWindow::on_port__valueChanged(int value)
{
    flagDirtyWindow();
    makeWriter();
}

void
MainWindow::on_systemId__valueChanged(int value)
{
    flagDirtyWindow();
}

void
MainWindow::on_rangeFormat__toggled(bool value)
{
    flagDirtyWindow();
}

void
MainWindow::on_manualRange__valueChanged(double value)
{
    flagDirtyWindow();
}

void
MainWindow::on_manualAzimuth__valueChanged(double value)
{
    flagDirtyWindow();
}

void
MainWindow::on_manualElevation__valueChanged(double value)
{
    flagDirtyWindow();
}

void
MainWindow::on_send__clicked()
{
    updateTargetPositionRAE(true);
}

void
MainWindow::on_actionSend__triggered()
{
    updateTargetPositionRAE(true);
}

void
MainWindow::on_startStop__clicked()
{
    if (!timer_->isActive()) {
        startTimer();
    } else {
        stopTimer();
    }
}

void
MainWindow::on_actionStart__triggered()
{
    on_startStop__clicked();
}

void
MainWindow::startTimer()
{
    if (elapsedTime_ && !model_->getActiveSegment(elapsedTime_)) {
        elapsedTime_ = 0.0;
        elapsed_->setText("0.0");
        reportCounter_ = 0;
        model_->rewind();
    }

    timer_->start();
    startStop_->setText("Stop");
    actionStart_->setText("Stop");
    model_->getActiveSegment(elapsedTime_);
    if (elapsedTime_)
        updateTargetPositionXYZ(true);
    else
        updateTargetPositionRAE(true);

    updateButtons();
    statusBar()->showMessage("Started");
}

void
MainWindow::stopTimer()
{
    timer_->stop();
    startStop_->setText("Start");
    actionStart_->setText("Stop");
    model_->rewind();
    updateButtons();
    statusBar()->showMessage("Stopped");
}

void
MainWindow::on_rewind__clicked()
{
    elapsedTime_ = 0.0;
    elapsed_->setText("0.0");
    model_->rewind();
    updateTargetPositionRAE(false);
    updateButtons();
    statusBar()->showMessage("Rewound to start");
}

void
MainWindow::on_actionRewind__triggered()
{
    on_rewind__clicked();
}

void
MainWindow::on_rate__valueChanged(double value)
{
    setTimerInterval(value);
    flagDirtyWindow();
    statusBar()->showMessage("New emission rate", 5000);
}

void
MainWindow::setTimerInterval(double hz)
{
    int msecs = int((1.0 / hz) * 1000);
    timer_->setInterval(msecs);
}

void
MainWindow::on_addSegment__clicked()
{
    QItemSelectionModel* selectionModel = segments_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    int row = index.isValid() ? (index.row() + 1) : 0;
    model_->addRow(row);
    index = model_->index(row, 0);
    segments_->setCurrentIndex(index);
    segments_->edit(index);
    setWindowModified(true);
    updateButtons();
}

void
MainWindow::on_moveUp__clicked()
{
    QItemSelectionModel* selectionModel = segments_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    if (index.isValid()) {
        int row = index.row();
        model_->moveUp(row);
        segments_->setCurrentIndex(model_->index(row - 1, index.column()));
        setWindowModified(true);
        updateButtons();
    }
}

void
MainWindow::on_moveDown__clicked()
{
    QItemSelectionModel* selectionModel = segments_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    if (index.isValid()) {
        int row = index.row();
        model_->moveDown(row);
        segments_->setCurrentIndex(model_->index(row + 1, index.column()));
        setWindowModified(true);
        updateButtons();
    }
}

void
MainWindow::on_removeSegment__clicked()
{
    QItemSelectionModel* selectionModel = segments_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    if (index.isValid()) {
        model_->deleteRow(index.row());
        setWindowModified(true);
        updateButtons();
    }
}

void
MainWindow::simulatorTimeout()
{
    double dt = timer_->interval() / 1000.0;
    elapsedTime_ += dt;
    elapsed_->setText(QString::number(elapsedTime_, 'f', 3));

    Segment* segment = model_->getActiveSegment(elapsedTime_);
    if (!segment) {
        stopTimer();
        return;
    }

    // Calculate the number of seconds under acceleration for this segment.
    //
    double t = segment->duration_ - (segment->end_ - elapsedTime_);
    xyz_[0] += (segment->xVel_ + segment->xAcc_ * t) * dt;
    xyz_[1] += (segment->yVel_ + segment->yAcc_ * t) * dt;
    xyz_[2] += (segment->zVel_ + segment->zAcc_ * t) * dt;

    updateTargetPositionXYZ(true);
}

void
MainWindow::updateTargetPositionRAE(bool send)
{
    Messages::TSPI::Ref msg = Messages::TSPI::MakeRAE(
        "RangeTruthEmitter", Messages::TSPI::GetSystemIdTag(systemId_->value()), Time::TimeStamp::Now().asDouble(),
        manualRange_->value() * 1000.0, Utils::degreesToRadians(manualAzimuth_->value()),
        Utils::degreesToRadians(manualElevation_->value()));
    updateTargetPosition(msg, send);
}

void
MainWindow::updateTargetPositionXYZ(bool send)
{
    Messages::TSPI::Ref msg =
        Messages::TSPI::MakeXYZ("RangeTruthEmitter", Messages::TSPI::GetSystemIdTag(systemId_->value()),
                                Time::TimeStamp::Now().asDouble(), xyz_[0], xyz_[1], xyz_[2]);
    updateTargetPosition(msg, send);
}

void
MainWindow::updateTargetPosition(const Messages::TSPI::Ref& msg, bool send)
{
    latitude_->setText(QString("%1 deg").arg(Utils::radiansToDegrees(msg->getLatitude())));
    longitude_->setText(QString("%1 deg").arg(Utils::radiansToDegrees(msg->getLongitude())));
    height_->setText(QString("%1 m").arg(msg->getHeight()));

    e_->setText(QString::number(msg->getE()));
    f_->setText(QString::number(msg->getF()));
    g_->setText(QString::number(msg->getG()));

    range_->setText(QString("%1 km").arg(msg->getRange() / 1000.0));
    azimuth_->setText(QString("%1 deg").arg(Utils::radiansToDegrees(msg->getAzimuth())));
    elevation_->setText(QString("%1 deg").arg(Utils::radiansToDegrees(msg->getElevation())));

    xyz_ = msg->getXYZ();
    x_->setText(QString("%1 m").arg(xyz_[0]));
    y_->setText(QString("%1 m").arg(xyz_[1]));
    z_->setText(QString("%1 m").arg(xyz_[2]));

    if (writer_ && send) {
        sendMessage(msg);

        QString txt = QString("Sent report %1").arg(++reportCounter_);
        if (timer_->isActive()) {
            double dt = timer_->interval() / 1000.0;
            txt += QString(" - next in %1 seconds").arg(dt);
        }

        statusBar()->showMessage(txt, 5000);
    }
}

void
MainWindow::sendMessage(const Messages::TSPI::Ref& msg)
{
    if (!writer_) return;

    if (!rangeFormat_->isChecked()) {
        IO::MessageManager mgr(msg);
        writer_->writeMessage(mgr);
        return;
    }

    union {
        int32_t full;
        unsigned char byte[1];
    } converter;

    tcnMsg_->reset();
    uint8_t* ptr = reinterpret_cast<uint8_t*>(tcnMsg_->base());
    ptr[0] = 0x01;
    ptr[1] = systemId_->value() >> 8;
    ptr[2] = systemId_->value() & 0xFF;

    uint32_t msecs = uint32_t(elapsedTime_ * 1000) % 60000;
    ptr[3] = uint8_t((msecs >> 8) & 0xFF);
    ptr[4] = uint8_t(msecs & 0xFF);

    double value = msg->getE();
    converter.full = int32_t(256.0 * value + (value < 0.0 ? -0.5 : 0.5));
    ptr[0 + 5] = converter.byte[0];
    ptr[1 + 5] = converter.byte[1];
    ptr[2 + 5] = converter.byte[2];
    ptr[3 + 5] = converter.byte[3];

    value = msg->getF();
    converter.full = int32_t(256.0 * value + (value < 0.0 ? -0.5 : 0.5));
    ptr[0 + 9] = converter.byte[0];
    ptr[1 + 9] = converter.byte[1];
    ptr[2 + 9] = converter.byte[2];
    ptr[3 + 9] = converter.byte[3];

    value = msg->getG();
    converter.full = int32_t(256.0 * value + (value < 0.0 ? -0.5 : 0.5));
    ptr[0 + 13] = converter.byte[0];
    ptr[1 + 13] = converter.byte[1];
    ptr[2 + 13] = converter.byte[2];
    ptr[3 + 13] = converter.byte[3];

    for (size_t index = 0; index < 3; ++index) ptr[index + 17] = 0x0;

    tcnMsg_->wr_ptr(20);

    writer_->writeEncoded(tcnMsg_->duplicate());
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    if (isWindowModified()) {
        QMessageBox::StandardButton button =
            QMessageBox::question(this, "Unsaved Changes",
                                  "<p><b>Do you want to save the changes to "
                                  "this configuration before closing?</b></p>"
                                  "<p>If you don't save, your changes will be "
                                  "lost.</p>",
                                  QMessageBox::Cancel | QMessageBox::Save | QMessageBox::Discard);

        if (button == QMessageBox::Cancel) {
            event->ignore();
            return;
        }

        if (button == QMessageBox::Save) {
            QString path = lastFile_;
            if (path.isEmpty()) {
                path = QFileDialog::getSaveFileName(this, "File to save", "", "Data (*.txt *.dat)");
                if (path.isEmpty()) {
                    event->ignore();
                    return;
                }
            }

            saveFile(path);
        }

        setWindowModified(false);
        updateButtons();
    }

    event->accept();

    if (timer_->isActive()) stopTimer();

    QSettings settings;
    settings.setValue("Address", address_->text());
    settings.setValue("Port", port_->value());
    settings.setValue("SystemId", systemId_->value());
    settings.setValue("Range", manualRange_->value());
    settings.setValue("Azimuth", manualAzimuth_->value());
    settings.setValue("Elevation", manualElevation_->value());
    settings.setValue("Rate", rate_->value());
    settings.setValue("RangeFormat", rangeFormat_->isChecked());

    Super::closeEvent(event);
}

void
MainWindow::updateButtons()
{
    bool enabled = model_->rowCount() > 0;
    startStop_->setEnabled(enabled);
    actionStart_->setEnabled(enabled);
    rewind_->setEnabled(enabled && elapsedTime_);
    actionRewind_->setEnabled(enabled && elapsedTime_);

    enabled = !timer_->isActive();
    address_->setEnabled(enabled);
    port_->setEnabled(enabled);
    send_->setEnabled(enabled);
    actionSend_->setEnabled(enabled);

    manualRange_->setEnabled(enabled);
    manualAzimuth_->setEnabled(enabled);
    manualElevation_->setEnabled(enabled);
    addSegment_->setEnabled(enabled);

    actionLoad_->setEnabled(enabled);
    actionSaveAs_->setEnabled(enabled);
    actionSave_->setEnabled(isWindowModified());
    actionRevert_->setEnabled(isWindowModified() && !lastFile_.isEmpty());

    QItemSelectionModel* selectionModel = segments_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    enabled &= index.isValid();
    removeSegment_->setEnabled(enabled);
    moveUp_->setEnabled(enabled && index.row() > 0);
    moveDown_->setEnabled(enabled && index.row() < model_->rowCount() - 1);
}

void
MainWindow::on_actionLoad__triggered()
{
    checkBeforeLoad();
    if (!isWindowModified()) {
        QString path = QFileDialog::getOpenFileName(this, "Choose a segment file", lastFile_, "Data (*.txt *.dat)");
        if (path.isEmpty()) {
            statusBar()->showMessage("Load canceled", 5000);
            return;
        }
        openFile(path);
    }
}

void
MainWindow::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        checkBeforeLoad();
        if (!isWindowModified()) {
            QString path(action->data().toString());
            openFile(path);
        }
    }
}

void
MainWindow::checkBeforeLoad()
{
    if (isWindowModified()) {
        QMessageBox::StandardButton button =
            QMessageBox::question(this, "Unsaved Changes",
                                  "<p><b>Do you want to save the changes to "
                                  "this configuration before loading a "
                                  "new one?</b></p>"
                                  "<p>If you don't save, your changes will be "
                                  "lost.</p>",
                                  QMessageBox::Cancel | QMessageBox::Save | QMessageBox::Discard);

        if (button == QMessageBox::Cancel) {
            return;
        } else if (button == QMessageBox::Save) {
            QString path = lastFile_;
            if (path.isEmpty()) {
                path = QFileDialog::getSaveFileName(this, "File to save", "", "Data (*.txt *.dat)");
                if (path.isEmpty()) { return; }
            }

            saveFile(path);
        } else {
            setWindowModified(false);
            updateButtons();
        }
    }
}

void
MainWindow::openFile(const QString& path)
{
    Logger::ProcLog log("openFile", Log());
    LOGINFO << "path: " << path << std::endl;

    if (timer_->isActive()) stopTimer();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        statusBar()->showMessage("Failed to open file!");
        return;
    }

    QByteArray line(file.readLine());
    if (line.isEmpty()) {
        statusBar()->showMessage("Empty file!");
        return;
    }

    QTextStream in(line);
    int port, systemId, rangeFormat;
    double rate, range, azimuth, elevation;
    QString address;
    in >> address >> port >> systemId >> rangeFormat >> range >> azimuth >> elevation >> rate;

    port_->setValue(port);
    systemId_->setValue(systemId);
    rate_->setValue(rate);
    rangeFormat_->setChecked(rangeFormat);
    if (!address.isEmpty()) address_->setText(address);

    manualRange_->setValue(range);
    manualAzimuth_->setValue(azimuth);
    manualElevation_->setValue(elevation);

    model_->clear();
    while (1) {
        QByteArray line(file.readLine());
        if (line.isEmpty()) break;

        QTextStream in(line);

        Segment* segment = new Segment;
        in >> segment->xVel_ >> segment->xAcc_ >> segment->yVel_ >> segment->yAcc_ >> segment->zVel_ >>
            segment->zAcc_ >> segment->duration_;

        model_->addRow(segment);
    }

    lastFile_ = path;
    setWindowTitle(QString("RangeTruthEmitter - %1[*]").arg(strippedName(path)));

    QSettings settings;
    QStringList files = settings.value("RecentFileList").toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > kMaxRecentFiles) files.removeLast();

    settings.setValue("RecentFileList", files);
    updateRecentFileActions();
    setWindowModified(false);
    updateButtons();

    statusBar()->showMessage("Loaded file", 5000);
}

void
MainWindow::on_actionSave__triggered()
{
    if (lastFile_.isEmpty()) {
        on_actionSaveAs__triggered();
    } else {
        saveFile(lastFile_);
    }
}

void
MainWindow::on_actionSaveAs__triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "File to save", lastFile_, "Data (*.txt *.dat)");

    if (path.isEmpty()) {
        statusBar()->showMessage("Save canceled", 5000);
        return;
    }

    saveFile(path);
}

void
MainWindow::saveFile(const QString& path)
{
    Logger::ProcLog log("saveFile", Log());
    LOGINFO << "path: " << path << std::endl;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        statusBar()->showMessage("Failed to open file!");
        return;
    }

    QTextStream out(&file);
    out << address_->text() << ' ' << port_->value() << ' ' << systemId_->value() << ' '
        << (rangeFormat_->isChecked() ? 1 : 0) << ' ' << manualRange_->value() << ' ' << manualAzimuth_->value() << ' '
        << manualElevation_->value() << ' ' << rate_->value() << '\n';

    model_->save(out);

    lastFile_ = path;
    setWindowTitle(QString("RangeTruthEmitter - %1[*]").arg(strippedName(path)));

    QSettings settings;
    QStringList files = settings.value("RecentFileList").toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > kMaxRecentFiles) files.removeLast();

    settings.setValue("RecentFileList", files);
    updateRecentFileActions();
    setWindowModified(false);
    updateButtons();

    statusBar()->showMessage("Saved file", 5000);
}

void
MainWindow::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("RecentFileList").toStringList();

    for (int index = 0; index < files.size(); ++index) {
        QString text(QString("&%1 %2").arg(index + 1).arg(strippedName(files[index])));
        recentFiles_[index]->setText(text);
        recentFiles_[index]->setData(files[index]);
        recentFiles_[index]->setVisible(true);
    }

    for (int index = files.size(); index < kMaxRecentFiles; ++index) recentFiles_[index]->setVisible(false);

    menuRecentFiles_->setEnabled(files.size() > 0);
}

void
MainWindow::on_actionClear__triggered()
{
    QSettings settings;
    settings.setValue("RecentFileList", QStringList());
    updateRecentFileActions();
}

void
MainWindow::on_actionRevert__triggered()
{
    if (isWindowModified()) {
        QMessageBox::StandardButton button = QMessageBox::question(this, "Reverting Changes",
                                                                   "<p><b>Do you want to revert to the last "
                                                                   "saved version of this file?</b></p> "
                                                                   "<p>If you do, your current changes will be "
                                                                   "lost.</p>",
                                                                   QMessageBox::No | QMessageBox::Yes);

        if (button == QMessageBox::No) { return; }

        openFile(lastFile_);
        statusBar()->showMessage("Reverted to saved file", 5000);
    }
}

QString
MainWindow::strippedName(const QString& path) const
{
    return QFileInfo(path).fileName();
}

void
MainWindow::flagDirtyWindow()
{
    if (!lastFile_.isEmpty()) {
        setWindowModified(true);
        updateButtons();
    }
}

void
MainWindow::on_address__editingFinished()
{
    Logger::ProcLog log("on_address__editingFinished", Log());
    QString address = address_->text();

    if (address.isEmpty()) {
        address_->setText(lastAddress_);
        return;
    }

    if (lastAddress_ != address) {
        makeWriter();
        if (!writer_) {
            QString msg = QString("Invalid multicast address %1/%2").arg(address_->text()).arg(port_->value());
            address_->setText(lastAddress_);
            makeWriter();
            statusBar()->showMessage(msg, 5000);
            return;
        }

        lastAddress_ = address;
        QSettings settings;
        settings.setValue("Address", address);
    }
}

void
MainWindow::makeWriter()
{
    if (writer_) removeWriter();
    writer_ = UDPMessageWriter::Make("RangeTruthEmitter", Messages::TSPI::GetMetaTypeInfo().getName(), address_->text(),
                                     port_->value());
    if (writer_) {
        writer_->start();
        statusBar()->showMessage(QString("TSPI writer active at %1/%2").arg(address_->text()).arg(port_->value()),
                                 5000);
    }
}

void
MainWindow::removeWriter()
{
    if (writer_) {
        delete writer_;
        writer_ = 0;
    }
}
