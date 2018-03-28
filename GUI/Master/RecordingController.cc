#include "QtCore/QDateTime"
#include "QtCore/QDir"
#include "QtCore/QProcess"
#include "QtGui/QApplication"
#include "QtGui/QCursor"
#include "QtGui/QHeaderView"
#include "QtGui/QItemSelectionModel"
#include "QtGui/QKeyEvent"
#include "QtGui/QMessageBox"
#include "QtNetwork/QTcpServer"

#include "GUI/LogUtils.h"
#include "GUI/Utils.h"
#include "GUI/modeltest.h"

#include "Alert.h"
#include "ConfigurationController.h"
#include "ConfigurationInfo.h"
#include "MainWindow.h"
#include "RecordingController.h"
#include "RecordingInfo.h"
#include "RecordingModel.h"
#include "RemoteClient.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

static const char* const kStart = "Start Recording";
static const char* const kStop = "Stop Recording";

Logger::Log&
RecordingController::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.RecordingController");
    return log_;
}

RecordingController::RecordingController(MainWindow& window) :
    QObject(&window), mainWindow_(window), model_(0), server_(0), activeRecording_(0), lastRecording_(0), prompt_(0)
{
    Logger::ProcLog log("RecordingController", Log());
    LOGINFO << std::endl;

    server_ = new QTcpServer(this);
    connect(server_, SIGNAL(newConnection()), SLOT(remoteConnection()));

    if (!server_->listen(QHostAddress::LocalHost, kServerPort)) {
        LOGERROR << "failed to open server socket" << std::endl;
    }

    model_ = new RecordingModel(this);
#ifdef __DEBUG__
    new ModelTest(model_, this);
#endif

    connect(model_, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            SLOT(updateColumns(const QModelIndex&, const QModelIndex&)));

    QTableView* recordings = mainWindow_.recordings_;

    recordings->setModel(model_);
    recordings->installEventFilter(this);
    recordings->viewport()->installEventFilter(this);
    recordings->setAlternatingRowColors(true);
    connect(recordings, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(editNotes(const QModelIndex&)));

    QHeaderView* header = recordings->horizontalHeader();
    header->setResizeMode(QHeaderView::Fixed);

    header = recordings->verticalHeader();
    header->hide();
    header->setMinimumSectionSize(-1);
    header->setResizeMode(QHeaderView::ResizeToContents);

    mainWindow_.recordingStartStop_->setText(kStart);
    mainWindow_.actionRecordingStartStop_->setText(kStart);
    mainWindow_.recordingStartStop_->setEnabled(false);
    mainWindow_.actionRecordingLimitDuration_->setEnabled(false);
    connect(mainWindow_.recordingStartStop_, SIGNAL(clicked()), SLOT(startStop()));

    connect(&mainWindow_, SIGNAL(nowTick(const QString&)), SLOT(doNowTick(const QString&)));
}

QStringList
RecordingController::restore(const ConfigurationController& controller)
{
    Logger::ProcLog log("restore", Log());

    QStringList names(controller.getAllConfigurations());
    LOGDEBUG << "num configs: " << names.size() << std::endl;

    QStringList dirs;
    foreach (QString name, names) {
        LOGDEBUG << "configuration: " << name << std::endl;
        ConfigurationInfo* cfg = controller.getConfigurationInfo(name);
        if (!dirs.contains(cfg->getRecordingsDirectory())) dirs << cfg->getRecordingsDirectory();
    }

    QStringList failed;
    foreach (QString path, dirs)
        failed += restore(path);

    return failed;
}

QStringList
RecordingController::restore(const QString& path)
{
    Logger::ProcLog log("restore", Log());
    LOGINFO << "path: " << path << std::endl;

    QStringList nameFilters;
    nameFilters << QDateTime::currentDateTime().toUTC().toString("yyyyMMdd-*");
    LOGDEBUG << "filter: " << nameFilters[0] << std::endl;

    QDir recordingDir(path);
    recordingDir.setNameFilters(nameFilters);

    QFileInfoList entries = recordingDir.entryInfoList(QDir::NoFilter, QDir::Name);
    LOGDEBUG << "num entries: " << entries.size() << std::endl;

    QStringList failed;
    foreach (QFileInfo fileInfo, entries) {
        LOGDEBUG << "recordingDir: " << fileInfo.fileName() << std::endl;
        QDir dir(fileInfo.filePath());
        RecordingInfo* info = model_->getModelData(dir.dirName());
        if (info) {
            info->addRecordingDirectory(dir);
        } else {
            info = new RecordingInfo(*this, dir);
            model_->add(info);
            if (info->getElapsedTime().isEmpty()) { failed << info->getName(); }

            // Get a sequence ID from the directory name, which is formatted as YYYYMMDD-HHMMSS-ID. Note that
            // the sequence ID may not exist for old-style recording names so play safe.
            //
            QStringList bits = dir.dirName().split("-");
            if (bits.size() > 2) {
                bool ok = false;
                int sequenceId = bits[2].toInt(&ok);
                if (ok && sequenceId >= mainWindow_.recordingSequenceId_->value())
                    mainWindow_.recordingSequenceId_->setValue(sequenceId + 1);
            }
        }
    }

    return failed;
}

void
RecordingController::doNowTick(const QString& now)
{
    if (activeRecording_) {
        if (activeRecording_->hasDurationPassed(now)) {
            stop(true);
        } else {
            model_->updateDuration();
            mainWindow_.setRecordingElapsed(activeRecording_->getFormattedElapsedTime(),
                                            activeRecording_->getRemainingTime());
        }
    }
}

void
RecordingController::remoteConnection()
{
    Logger::ProcLog log("remoteConnection", Log());
    LOGINFO << std::endl;
    QTcpSocket* client = server_->nextPendingConnection();
    if (client) new RemoteClient(*this, *client);
}

void
RecordingController::startStop()
{
    QStringList filter(mainWindow_.getRecordableConfigurations());

    if (!activeRecording_) {
        if (QMessageBox::question(qApp->activeWindow(), "Start Recording",
                                  QString("<p>Begin recording for the configurations:</p>"
                                          "<ul><li>%1</li></ul>"
                                          "<p>The recording process may introduce "
                                          "sensitive data onto the disk drives of the "
                                          "SideCar. Please make sure you have "
                                          "authorization to proceed.</p>"
                                          "<p>Begin the recording process?</p>")
                                      .arg(filter.join("</li><li>")),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
            showMessage("Start recording cancelled");
            return;
        }

        Status rc = start(filter);

        if (rc == kNoLoadedConfig) {
            showError("Recording Start", "No configurations to record.");
        } else if (rc == kFailedCreateDirectory) {
            showError("Recording Start", "Unable to create recording directory.");
        } else if (rc == kFailedRecordingSetup) {
            showError("Recording Start", "Failed to properly initialize recording directory.");
        }
    } else {
        prompt_ = new QMessageBox(QMessageBox::Question, "Stop Recording",
                                  "<p>Are you sure you want to stop the recording process?</p>",
                                  QMessageBox::No | QMessageBox::Yes, 0);

        prompt_->setDefaultButton(QMessageBox::No);

        connect(prompt_, SIGNAL(finished(int)), SLOT(promptDone(int)));

        prompt_->show();
    }
}

void
RecordingController::promptDone(int resultCode)
{
    Logger::ProcLog log("promptDone", Log());
    LOGINFO << resultCode << std::endl;

    if (resultCode == QMessageBox::Yes) {
        stop(false);
    } else {
        showMessage("Recording stop cancelled");
    }

    prompt_ = 0;
}

RecordingController::Status
RecordingController::start(const QStringList& configNames)
{
    Logger::ProcLog log("start", Log());
    LOGINFO << std::endl;

    if (configNames.empty()) {
        LOGERROR << "no configurations" << std::endl;
        return kNoLoadedConfig;
    }

    QStringList doneDirs;
    QStringList recordingPaths;

    // Create the name of the new recording. Our recording names have the follwoing format:
    //   YYYYMMDD-HHMMSS-ID[-CAL] where ID is an increasing sequence ID and CAL is an optional tag that
    //   indicates the recording is a calibration. NOTE: if you change the formatting of the recording names,
    //   you must change the Playback application as well since it relies on this formatting.
    //
    QString recordingName(QDateTime::currentDateTime().toUTC().toString("yyyyMMdd-hhmmss"));
    recordingName += QString("-%1").arg(mainWindow_.recordingSequenceId_->value());

    // Scan the configurations to see if any of them are in the calibration state. We only want to append 'CAL'
    // once.
    //
    foreach (QString configName, configNames) {
        if (mainWindow_.isCalibrating(configName)) {
            recordingName += "-CAL";
            break;
        }
    }

    QString recordingPath;
    QDir recordingDir;
    activeRecording_ = 0;

    // Prepare for a new recording. We visit each of the recordable configurations since each one may have a
    // different destination for recording.
    //
    foreach (QString configName, configNames) {
        ConfigurationInfo* configuration = mainWindow_.getConfigurationInfo(configName);

        // Get the recording directory to use.
        //
        QString parentDir(configuration->getRecordingsDirectory());

        // Only process the directory once, no matter how many configurations use it.
        //
        if (!doneDirs.contains(parentDir)) {
            // Attempt to create a new recording directory.
            //
            QFileInfo filePath(parentDir, recordingName);
            recordingPath = filePath.filePath();
            LOGDEBUG << "recordingPath: " << recordingPath << std::endl;

            QDir dir(parentDir);
            if (!dir.mkdir(recordingName)) {
                LOGERROR << "failed to create new recording directory" << std::endl;
                return kFailedCreateDirectory;
            }

            recordingDir.setPath(recordingPath);

            // Allow everyone to read/write to the new directory.
            //
            if (QProcess::execute(QString("chmod a+rw %1").arg(recordingPath)) != 0) return kFailedRecordingSetup;

            // Remove any old 'last' soft link.
            //
            if (QProcess::execute(QString("rm -f %1/last").arg(parentDir)) != 0) return kFailedRecordingSetup;

            // Create 'last' soft link to the new recording directory
            //
            if (QProcess::execute(QString("ln -s %1 %2/last").arg(recordingPath).arg(parentDir)) != 0)
                return kFailedRecordingSetup;

            if (!activeRecording_) {
                // Create a new RecordingInfo object to represent the pending recording.
                //
                QTime duration;
                if (mainWindow_.recordingDurationEnable_->isChecked())
                    duration = mainWindow_.recordingDuration_->time();
                activeRecording_ = new RecordingInfo(*this, recordingName, duration);
            }

            activeRecording_->addRecordingDirectory(recordingDir);
            doneDirs.append(parentDir);
        }

        // Copy over the configuration file to the new recording directory.
        //
        QFileInfo filePath(configuration->getPath());
        if (!QFile::copy(filePath.absoluteFilePath(), recordingDir.absoluteFilePath(filePath.fileName())))
            return kFailedRecordingSetup;

        // Save the config name and recording path for use later when we command the runners to begin recording.
        //
        recordingPaths.append(recordingPath);
        activeRecording_->addConfiguration(configName);
    }

    // Fetch any runtime parameter deviations from startup/configured values and add them to the notes window
    // for historical purposes.
    //
    QStringList parameterChanges;
    mainWindow_.getChangedParameters(configNames, parameterChanges);
    activeRecording_->start(parameterChanges);

    // Finally, command the tasks to start recording
    //
    if (!mainWindow_.startRecording(configNames, recordingPaths)) {
        LOGERROR << "failed to command runners to start recording" << std::endl;
        return kFailedPostRecordingStateChange;
    }

    activeRecording_->showNotesWindow();
    lastRecording_ = activeRecording_;

    model_->add(activeRecording_);
    mainWindow_.recordings_->scrollToBottom();

    // Update GUI to reflect the change in recording state.
    //
    mainWindow_.recordingStartStop_->setText(kStop);
    mainWindow_.actionRecordingStartStop_->setText(kStop);
    mainWindow_.setRecordingElapsed(activeRecording_->getFormattedElapsedTime(), activeRecording_->getRemainingTime());
    mainWindow_.recordingDurationEnable_->setEnabled(false);
    mainWindow_.actionRecordingLimitDuration_->setEnabled(false);
    mainWindow_.recordingDuration_->setEnabled(false);
    mainWindow_.recordingSequenceId_->stepUp();

    LOGINFO << "started recording" << std::endl;

    emit statusChanged(configNames, true);

    return kOK;
}

RecordingController::Status
RecordingController::start()
{
    return start(mainWindow_.getRecordableConfigurations());
}

RecordingController::Status
RecordingController::stop(bool byDuration)
{
    Logger::ProcLog log("stop", Log());

    Q_ASSERT(activeRecording_);

    // Stop the recording process first before anything else.
    //
    QStringList configNames(activeRecording_->getConfigurationNames());
    RecordingController::Status rc = kOK;
    if (!mainWindow_.stopRecording(configNames)) {
        LOGERROR << "failed to command runners to stop recording" << std::endl;
        rc = kFailedPostRecordingStateChange;
    }

    mainWindow_.recordingStartStop_->setText(kStart);
    mainWindow_.actionRecordingStartStop_->setText(kStart);
    mainWindow_.recordingDurationEnable_->setEnabled(true);
    mainWindow_.actionRecordingLimitDuration_->setEnabled(true);
    mainWindow_.recordingDuration_->setEnabled(true);

    if (byDuration) {
        if (prompt_) delete prompt_;
        prompt_ = new QMessageBox(QMessageBox::Information, "Recording Stopped",
                                  QString("<p>Recording %1 automatically stopped due to duration "
                                          "setting.</p>")
                                      .arg(activeRecording_->getName()),
                                  QMessageBox::Ok, qApp->activeWindow());
        prompt_->show();
        prompt_ = 0;
    }

    activeRecording_->recordingStopped();
    activeRecording_ = 0;
    emit statusChanged(configNames, false);

    LOGINFO << "stopped recording" << std::endl;
    return rc;
}

void
RecordingController::editNotes(const QModelIndex& index)
{
    RecordingInfo* obj = model_->getModelData(index.row());
    if (obj) obj->showNotesWindow();
}

bool
RecordingController::eventFilter(QObject* object, QEvent* event)
{
    if (object == mainWindow_.recordings_) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace) {
                deleteSelectedRecording();
            }
        } else if (event->type() == QEvent::Resize) {
            adjustColumnSizes();
        }
    } else if (object == mainWindow_.recordings_->viewport()) {
        if (event->type() == QEvent::Resize) { adjustColumnSizes(); }
    }
    return Super::eventFilter(object, event);
}

void
RecordingController::deleteSelectedRecording()
{
    // Obtain the current selection
    //
    QItemSelectionModel* selectionModel = mainWindow_.recordings_->selectionModel();
    QModelIndex index = selectionModel->currentIndex();
    if (!index.isValid()) return;

    // Only allow deletion if it is not recording.
    //
    RecordingInfo* obj = model_->getModelData(index.row());
    if (!obj->isDone()) return;

    // Make sure the user really wants to do this!
    //
    if (QMessageBox::question(qApp->activeWindow(), "Delete Recording",
                              QString("<p>Delete Recording</p>"
                                      "<p>Are you sure you want to delete "
                                      "the recording '%1' and all of the "
                                      "data files it contains?</p>")
                                  .arg(obj->getName()),
                              QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (obj == lastRecording_) lastRecording_ = 0;

    // Get the list of directories we need to remove. There may be more than one since multiple configuration
    // files may have been involved in the recording with different recording base paths. Remove the
    // directories.
    //
    QStringList dirPaths(obj->getRecordingDirectories());
    foreach (QDir dir, dirPaths)
        RemoveDirectory(dir);

    // Remove the item from the model and attempt to select another entry.
    //
    selectionModel->clear();
    model_->remove(index);
    if (model_->rowCount() > 0) {
        if (model_->rowCount() <= index.row()) index = model_->index(model_->rowCount() - 1, 0);
        selectionModel->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    }

    QApplication::restoreOverrideCursor();
}

void
RecordingController::showMessage(const QString& text, int duration) const
{
    mainWindow_.showMessage(text, duration);
}

QString
RecordingController::getNow() const
{
    return mainWindow_.getNow();
}

void
RecordingController::updateColumns(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    adjustColumnSizes();
}

bool
RecordingController::showError(const QString& title, const QString& text)
{
    Logger::ProcLog log("ShowError", Log());
    LOGINFO << text << std::endl;
    Alert::ShowWarning(&mainWindow_, title, text);
    return false;
}

void
RecordingController::reset()
{
    if (activeRecording_) stop(false);

    if (lastRecording_) lastRecording_ = 0;
}

void
RecordingController::adjustColumnSizes()
{
    Logger::ProcLog log("adjustColumnSizes", Log());
    LOGINFO << "begin" << std::endl;

    // Approach: fetch the width of the view port, subtract the column width for all columns but the first one
    // (name), and allocate the remaining space to the first one if it is greater than its minimum width.
    //
    QTableView* recordings = mainWindow_.recordings_;
    recordings->resizeColumnsToContents();
    int minimum = recordings->columnWidth(RecordingModel::kConfig);
    int available(recordings->viewport()->width());
    for (int column = 0; column < model_->columnCount(); ++column)
        if (column != RecordingModel::kConfig) available -= recordings->columnWidth(column);
    if (available >= minimum) recordings->setColumnWidth(RecordingModel::kConfig, available);
}

void
RecordingController::restoreActiveRecording()
{
    if (model_->rowCount() > 0) { activeRecording_ = model_->getModelData(model_->rowCount() - 1); }
}
