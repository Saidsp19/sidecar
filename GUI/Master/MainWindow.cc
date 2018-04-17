#include <cmath>
#include <time.h>

#include "QtCore/QDateTime"
#include "QtCore/QList"
#include "QtCore/QSettings"
#include "QtCore/QTime"
#include "QtGui/QCloseEvent"
#include "QtWidgets/QFileDialog"
#include "QtWidgets/QMessageBox"
#include "QtWidgets/QProgressBar"
#include "QtWidgets/QProgressDialog"
#include "QtWidgets/QStatusBar"

#include "GUI/LogUtils.h"
#include "GUI/ToolBar.h"
#include "GUI/modeltest.h"
#include "IO/ProcessingState.h"

#include "App.h"
#include "ConfigurationController.h"
#include "ConfigurationInfo.h"
#include "ConfigurationSettings.h"
#include "MainWindow.h"
#include "RecordingController.h"
#include "RecordingInfo.h"
#include "RecordingSettings.h"
#include "ServicesModel.h"
#include "Settings.h"
#include "StatusCollector.h"
#include "StatusWidget.h"
#include "TreeViewItem.h"
#include "UtilsWidget.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.MainWindow");
    return log_;
}

MainWindow::MainWindow() :
    MainWindowBase(), Ui::MainWindow(), statusWidget_(new StatusWidget(this)), utilsWidget_(new UtilsWidget(this)),
    servicesModel_(0), browser_(0), configurationController_(0), recordingController_(0), checker_(), nowTimerId_(-1),
    lastFailureCount_(0), preDrops_(0), preDupes_(0)
{
    Logger::ProcLog log("MainWindow", Log());
    LOGDEBUG << std::endl;
    setupUi(this);
    setObjectName("MainWindow");
#ifdef __DEBUG__
    setWindowTitle("Master (DEBUG)");
#endif
    setAttribute(Qt::WA_DeleteOnClose);
    connect(utilsWidget_->runState_, SIGNAL(activated(int)), SLOT(changeRunState(int)));
    connect(utilsWidget_->clearStats_, SIGNAL(clicked()), SLOT(clearStats()));
    connect(utilsWidget_->find_, SIGNAL(textChanged(const QString&)), SLOT(filterRunners()));

    showRecordingState(false);
    splitter1->setSizes(QList<int>() << 300 << 500);
    splitter2->setSizes(QList<int>() << 200 << 500);

    struct tm* bits;
    time_t now;
    ::time(&now);
    bits = ::localtime(&now);
    zoneName_ = bits->tm_zone;

    servicesModel_ = new ServicesModel(this);
#ifdef __DEBUG__
    new ModelTest(servicesModel_, this);
#endif
    runners_->setModel(servicesModel_);

    connect(servicesModel_, SIGNAL(statusUpdated()), SLOT(servicesStatusUpdated()));

    browser_ = new ServiceBrowser(this, StatusCollector::GetCollectorType());
    connect(browser_, SIGNAL(availableServices(const ServiceEntryHash&)),
            SLOT(updateActiveMasters(const ServiceEntryHash&)));
    browser_->start();

    App* app = getApp();
    Settings& settings = app->getSettings();

    settings.getRecordingSettings().connectWidgets(recordingDurationEnable_, recordingDuration_);

    configurationController_ = new ConfigurationController(*this, settings.getConfigurationSettings());
    connect(configurationController_, SIGNAL(activeConfigurationsChanged(const QStringList&)),
            SLOT(configurationStatusChanged(const QStringList&)));
    connect(configurationController_, SIGNAL(selectedConfigurationChanged(const ConfigurationInfo*)),
            SLOT(updateRecordingStartStop()));

    connect(servicesModel_, SIGNAL(runnerAdded(const RunnerItem*)), configurationController_,
            SLOT(foundRunner(const RunnerItem*)));

    connect(servicesModel_, SIGNAL(runnerRemoved(const RunnerItem*)), configurationController_,
            SLOT(lostRunner(const RunnerItem*)));

    recordingController_ = new RecordingController(*this);
    connect(recordingController_, SIGNAL(statusChanged(const QStringList&, bool)), configurationController_,
            SLOT(recordingStatusChanged(const QStringList&, bool)));

    actionRecordingLimitDuration_->setCheckable(true);
    actionRecordingLimitDuration_->setChecked(recordingDurationEnable_->isChecked());

    connect(actionRecordingLimitDuration_, SIGNAL(triggered(bool)), recordingDurationEnable_, SLOT(setChecked(bool)));
    connect(recordingDurationEnable_, SIGNAL(clicked(bool)), actionRecordingLimitDuration_, SLOT(setChecked(bool)));
    connect(actionRecordingStartStop_, SIGNAL(triggered()), recordingStartStop_, SLOT(click()));

    ToolBar* toolBar = makeToolBar("Status", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    toolBar->addWidget(statusWidget_);

    toolBar = makeToolBar("Utils", Qt::BottomToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    toolBar->addWidget(utilsWidget_);

    QAction* action = app->getToolsMenuAction(App::kShowConfigurationWindow);
    connect(showConfigurationWindow_, SIGNAL(clicked()), action, SLOT(trigger()));

    filterRunners();

    getApp()->installEventFilter(this);
}

void
MainWindow::updateActiveMasters(const ServiceEntryHash& found)
{
    // The number of running Master applications (including us) has changed.
    //
    Logger::ProcLog log("updateActiveMasters", Log());
    activeMastersCount_ = found.size();
    statusWidget_->activeMasters_->setNum(activeMastersCount_);
}

void
MainWindow::doNowTick()
{
    if (checker_) processChecker();
    now_ = QDateTime::currentDateTime().toUTC().time().toString("hh:mm:ss") + " UTC";
    statusWidget_->now_->setText(now_);
    emit nowTick(now_);
}

void
MainWindow::processChecker()
{
    Logger::ProcLog log("processChecker", Log());
    LOGINFO << std::endl;

    // !!! Protect against reentrancy. Calling QProgressDialog::setValue() will process the event queue because
    // !!! it has Qt::ApplicationModal modality. Doing this will ensure that doNowTick() will not call us if the
    // !!! timer fires while we are in this routine.
    //
    QProgressDialog* tmp = checker_;
    checker_ = 0;

    // If there are active runners or we have spent enough time looking for them, cancel the check.
    //
    int nextValue = tmp->value() + 1;
    if (servicesModel_->rowCount() || nextValue == tmp->maximum()) tmp->cancel();

    if (!tmp->wasCanceled()) {
        // Update the progress dialog
        //
#ifdef darwin
        tmp->setLabelText(QString("Searching for active runners...\n%1%").arg(int(::rint(nextValue / 0.8))));
#endif
        tmp->setValue(nextValue);

        // !!! Now it is safe to restore this pointer.
        //
        checker_ = tmp;
    } else {
        // Done with the progress dialog.
        //
        tmp->deleteLater();

        // Make ourselves the active window and load data from any recent recordings.
        //
        activateWindow();
        restoreRecordings();

        // Determine if any recording directories are not valid, and alert the reader if so.
        //
        QStringList invalid = configurationController_->getInvalidRecordingDirectories();
        if (!invalid.empty()) {
            QMessageBox::critical(this, "Invalid Recording Directories",
                                  QString("<p>The following recording "
                                          "directories are not valid. "
                                          "Configurations using them "
                                          "have been disabled for "
                                          "recording purposes.</p>"
                                          "<ul><li>%1</li></ul></p>")
                                      .arg(invalid.join("</li><li>")));
        }

        // If we are already recording, attempt to restore recording state.
        //
        if (configurationController_->isAnyRecording()) {
            showRecordingState(true);
            recordingController_->restoreActiveRecording();
        }
    }
}

void
MainWindow::restoreConfigurations()
{
    statusBar()->showMessage("Loading configuration files...");
    QStringList failures = configurationController_->restore();
    if (!failures.isEmpty()) {
        QMessageBox::critical(this, "Invalid Configuration Files",
                              QString("<p>Failed to properly read in the "
                                      "following configuration files:</p>"
                                      "<ul><li>%1</li></ul></p>"
                                      "<p>Move the mouse cursor pointer over "
                                      "configuration name or status entry "
                                      "to see the error text.</p>")
                                  .arg(failures.join("</li><li>")));
    }
}

void
MainWindow::restoreRecordings()
{
    statusBar()->showMessage("Loading previous recordings information...");
    QStringList failures = recordingController_->restore(*configurationController_);
    if (!failures.isEmpty()) {
        QMessageBox::critical(this, "Invalid Recording Files",
                              QString("<p>Failed to properly read in the "
                                      "'notes.txt' file for the following "
                                      "recordings:</p>"
                                      "<ul><li>%1</li></ul></p>")
                                  .arg(failures.join("</li><li>")));
    }

    QApplication::restoreOverrideCursor();
    showMessage("Ready");
}

void
MainWindow::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == nowTimerId_) {
        doNowTick();
    } else {
        Super::timerEvent(event);
    }
}

bool
MainWindow::shutdownConfiguration(const QString& configName)
{
    Logger::ProcLog log("shutdownConfiguration", Log());
    LOGINFO << std::endl;

    ConfigurationInfo* config = configurationController_->getConfigurationInfo(configName);

    if (config->isRecording()) {
        if (QMessageBox::question(this, "Shutdown Pending",
                                  QString("<p>A recording is in progress.</p>"
                                          "<p>Do you wish to stop recording and "
                                          "shutdown the SideCar processing "
                                          "streams?</p>"),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
            showMessage("Shutdown canceled.");
            return false;
        }

        stopRecording();
    } else if (config->getStatus() == ConfigurationInfo::kRunning) {
        if (QMessageBox::question(this, "Shutdown Pending",
                                  QString("<p>Shut down configuration '%1'?</p>"
                                          "<p>Are you sure you want to shutdown the "
                                          "SideCar processing streams for the '%1' "
                                          "configuration?</p>")
                                      .arg(configName),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes) == QMessageBox::No) {
            showMessage("Shutdown canceled.");
            return false;
        }
    }

    showMessage("Shutting down remote runner processes.");
    forceShutdown(config);

    return true;
}

void
MainWindow::forceShutdown(ConfigurationInfo* config)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    if (utilsWidget_->find_->text() != "") utilsWidget_->find_->setText("");

    config->shuttingDown();
    if (servicesModel_->postShutdownRequest(config->getName())) {
        showMessage("Posted shutdown request.");
    } else {
        showMessage("Failed to post shutdown request.");
    }

    QApplication::restoreOverrideCursor();
}

void
MainWindow::restoreFromSettings(QSettings& settings)
{
    Super::restoreFromSettings(settings);
    QByteArray state = settings.value("Splitter1").toByteArray();
    if (!state.isNull()) splitter1->restoreState(state);
    state = settings.value("Splitter2").toByteArray();
    if (!state.isNull()) splitter2->restoreState(state);
}

void
MainWindow::saveToSettings(QSettings& settings)
{
    Super::saveToSettings(settings);
    settings.setValue("Splitter1", splitter1->saveState());
    settings.setValue("Splitter2", splitter2->saveState());
}

void
MainWindow::configurationStatusChanged(const QStringList& active)
{
    bool hasActive = !active.empty();
    utilsWidget_->runState_->setEnabled(hasActive);
    utilsWidget_->runState_->setCurrentIndex(-1);
    utilsWidget_->clearStats_->setEnabled(hasActive);
    statusWidget_->activeConfigs_->setNum(active.size());
    filterRunners();
    updateRecordingStartStop();
}

bool
MainWindow::isAnyActive() const
{
    return statusWidget_->activeConfigs_->text() != "0";
}

void
MainWindow::showMessage(const QString& text, int duration)
{
    statusBar()->clearMessage();
    statusBar()->showMessage(text, duration);
}

void
MainWindow::servicesStatusUpdated()
{
    static Logger::ProcLog log("servicesStatusUpdated", Log());

    QStringList filter(configurationController_->getViewableConfigurations());

    int runnerCount = 0;
    int streamCount = 0;
    int pendingCount = 0;
    int failureCount = 0;
    servicesModel_->getServiceStats(runnerCount, streamCount, pendingCount, failureCount);

    statusWidget_->activeRunners_->setNum(runnerCount);
    statusWidget_->activeStreams_->setNum(streamCount);
    statusWidget_->pending_->setNum(pendingCount);
    statusWidget_->failures_->setNum(failureCount);

    LOGDEBUG << failureCount << ' ' << lastFailureCount_ << std::endl;

    if (failureCount != lastFailureCount_) {
        lastFailureCount_ = failureCount;
        QPalette palette(statusWidget_->failures_->palette());
        palette.setColor(QPalette::WindowText, failureCount > 0 ? TreeViewItem::GetFailureColor() : Qt::black);
        palette.setColor(QPalette::ButtonText, failureCount > 0 ? TreeViewItem::GetFailureColor() : Qt::black);
        statusWidget_->failures_->setPalette(palette);
        statusWidget_->failuresLabel_->setPalette(palette);
    }

    if (recordingController_) {
        RecordingInfo* activeRecording = recordingController_->getActiveRecording();
        if (activeRecording) {
            filter = activeRecording->getConfigurationNames();
            int drops = 0;
            int dupes = 0;
            servicesModel_->getDropsAndDupes(filter, drops, dupes);
            activeRecording->updateStats(drops - preDrops_, dupes - preDupes_);
        }
    }
}

void
MainWindow::updateRecordingStartStop()
{
    bool enabled = recordingController_ &&
                   (recordingController_->getActiveRecording() ||
                    (configurationController_ && !configurationController_->getRecordableConfigurations().empty()));
    recordingStartStop_->setEnabled(enabled);
    actionRecordingStartStop_->setEnabled(enabled);
}

void
MainWindow::changeRunState(int index)
{
    Logger::ProcLog log("changeRunState", Log());
    QStringList filter(configurationController_->getViewableRunningConfigurations());

    if (index == -1) return;

    utilsWidget_->runState_->setCurrentIndex(-1);

    if (servicesModel_->isRecording(filter)) {
        if (filter.size() > 1) {
            if (QMessageBox::question(this, "Recording In Progress",
                                      QString("<p>The following configurations are "
                                              "recording</p><ul><li>%1</li>/</ul>"
                                              "<p>Changing the processing states of the "
                                              "active runners may affect the contents of "
                                              "the recording files. Continue?</p>")
                                          .arg(filter.join("</li><li>")),
                                      QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                showMessage("State change canceled.");
                return;
            }
        } else {
            if (QMessageBox::question(this, "Recording In Progress",
                                      QString("<p>Configuration %1 is recording.</p>"
                                              "<p>Changing the processing states of the "
                                              "active runners may affect the contents of "
                                              "the recording files. Continue?</p>")
                                          .arg(filter[0]),
                                      QMessageBox::No | QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                showMessage("State change canceled.");
                return;
            }
        }
    }

    IO::ProcessingState::Value state(IO::ProcessingState::Value(IO::ProcessingState::kAutoDiagnostic + index));

    if (servicesModel_->postProcessingStateChange(filter, state)) {
        statusBar()->clearMessage();
        showMessage(QString("State changed to %1.").arg(IO::ProcessingState::GetName(state)));
    } else {
        showMessage("State change failed.");
    }
}

bool
MainWindow::startRecording(const QStringList& configNames, const QStringList& recordingPaths)
{
    Logger::ProcLog log("startRecording", Log());
    LOGINFO << std::endl;

    if (utilsWidget_->find_->text() != "") utilsWidget_->find_->setText("");

    if (!servicesModel_->postRecordingStart(configNames, recordingPaths)) {
        QMessageBox::critical(qApp->activeWindow(), "Start Recording Failure",
                              "<p>Failed to command one or more remote runners to start "
                              "recording.</p>");
        showMessage("Recording start failed.");
        return false;
    }

    servicesModel_->getDropsAndDupes(configNames, preDrops_, preDupes_);
    showRecordingState(true);
    showMessage("Recording started.");

    return true;
}

bool
MainWindow::stopRecording()
{
    return stopRecording(recordingController_->getActiveRecording()->getConfigurationNames());
}

bool
MainWindow::getChangedParameters(const QStringList& configNames, QStringList& changes) const
{
    return servicesModel_->getChangedParameters(configNames, changes);
}

bool
MainWindow::stopRecording(const QStringList& configNames)
{
    Logger::ProcLog log("stopRecording", Log());
    LOGINFO << std::endl;

    if (utilsWidget_->find_->text() != "") utilsWidget_->find_->setText("");

    showRecordingState(false);

    if (!servicesModel_->postRecordingStop(configNames)) {
        QMessageBox::critical(qApp->activeWindow(), "Stop Recording Failure",
                              "<p>Failed to command one or more remote runners to stop "
                              "recording.</p>");
        showMessage("Recording stop failed.");
        return false;
    }

    showMessage("Recording stopped.");

    return true;
}

void
MainWindow::showRecordingState(bool recording)
{
    QColor color = recording ? TreeViewItem::GetRecordingColor() : Qt::black;
    QPalette palette(recordingStartStop_->palette());
    palette.setColor(QPalette::Active, QPalette::WindowText, color);
    palette.setColor(QPalette::Active, QPalette::ButtonText, color);
    palette.setColor(QPalette::Inactive, QPalette::WindowText, color);
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, color);
    recordingStartStop_->setPalette(palette);

    if (recording) {
        recordingStartStop_->setText("Stop Recording");
        recordingStartStop_->setToolTip("Stop current recording");
        actionRecordingStartStop_->setText("Stop Recording");
        utilsWidget_->clearStats_->setEnabled(false);
    } else {
        recordingStartStop_->setText("Start Recording");
        recordingStartStop_->setToolTip("Start new recording");
        actionRecordingStartStop_->setText("Start Recording");
        utilsWidget_->clearStats_->setEnabled(true);
    }

    statusWidget_->now_->setPalette(palette);
    statusWidget_->elapsed_->setPalette(palette);
    statusWidget_->elapsed_->setVisible(recording);

    configurationsControls_->setEnabled(!recording);

    getApp()->setRecording(recording);
}

void
MainWindow::showEvent(QShowEvent* event)
{
    Logger::ProcLog log("showEvent", Log());
    LOGINFO << nowTimerId_ << ' ' << checker_ << std::endl;

    Super::showEvent(event);

    if (nowTimerId_ == -1) {
        // Load in the configuration files that were last used
        //
        restoreConfigurations();

        if (!servicesModel_->start()) {
            LOGERROR << "failed to start the status collector" << std::endl;
            QMessageBox::critical(this, "Startup",
                                  QString("<p>Failed to start the status "
                                          "collector thread. Unable to "
                                          "continue."));
            QApplication::quit();
        }

        // Create a progress dialog to tell the user that we are waiting for runners to appear. Since our clock
        // timer will fire ever 0.1 seconds and we want to allow 8 seconds for runners to appear, we need a
        // progress maximum of 80.
        //
#ifdef darwin
        checker_ = new QProgressDialog("Searching for active runners...\n0%", "Cancel", 0, 80, this);
        checker_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        checker_->setSizeGripEnabled(false);
#else
        checker_ = new QProgressDialog("Searching for active runners...", "Cancel", 0, 80, this);
        checker_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        checker_->setSizeGripEnabled(false);
#endif
        checker_->setValue(0);
        checker_->setMinimumDuration(0);
        checker_->setWindowModality(Qt::ApplicationModal);
        checker_->setAutoClose(true);
        QApplication::setOverrideCursor(Qt::WaitCursor);
        nowTimerId_ = startTimer(100);
    }
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    Logger::ProcLog log("closeEvent", Log());
    LOGINFO << std::endl;
    App* app = getApp();
    if (!app->isQuitting()) {
        event->ignore();
        QTimer::singleShot(0, app, SLOT(applicationQuit()));
        return;
    }

    killTimer(nowTimerId_);
    nowTimerId_ = 0;
    Super::closeEvent(event);
}

void
MainWindow::setRecordingElapsed(const QString& elapsed, const QString& remaining)
{
    statusWidget_->elapsed_->setText(elapsed + remaining);
}

QStringList
MainWindow::getRecordableConfigurations() const
{
    return configurationController_->getRecordableConfigurations();
}

ConfigurationInfo*
MainWindow::getConfigurationInfo(const QString& name) const
{
    return configurationController_->getConfigurationInfo(name);
}

bool
MainWindow::isCalibrating(const QString& name) const
{
    return servicesModel_->isCalibrating(QStringList(name));
}

void
MainWindow::filterRunners()
{
    if (configurationController_) {
        runners_->setConfigurationVisibleFilter(configurationController_->getAllConfigurations(),
                                                configurationController_->getViewableConfigurations(),
                                                utilsWidget_->find_->text());
    }
}

void
MainWindow::clearStats()
{
    Logger::ProcLog log("clearStats", Log());
    QStringList filter(configurationController_->getViewableRunningConfigurations());

    if (servicesModel_->postClearStats(filter)) {
        showMessage("Cleared processing statistics.");
    } else {
        showMessage("Failed to clear processing statistics.");
    }
}

void
MainWindow::parameterValuesChanged(const QStringList& changes)
{
    RecordingInfo* recordingInfo = recordingController_->getActiveRecording();
    if (recordingInfo) recordingInfo->addChangedParameters(changes);
}

bool
MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    static Logger::ProcLog log("eventFilter", Log());

    // Trap KeyPress and KeyRelease events so we can dynamically update the configStartStop_ button when the
    // user presses the ALT modifier key.
    //
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Shift) {
        // User is pressing the ALT modifier. Append 'All' to the button text if not already present.
        //
        QStringList bits = configStartStop_->text().split(' ');
        if (bits.size() == 1) { configStartStop_->setText(bits[0] + " All"); }
    } else if ((event->type() == QEvent::KeyRelease && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Shift) ||
               (event->type() == QEvent::ApplicationDeactivate)) {
        // User released the ALT modifier. Remove 'All' from the button text if not already present.
        //
        QStringList bits = configStartStop_->text().split(' ');
        if (bits.size() == 2) { configStartStop_->setText(bits[0]); }
    }

    return QObject::eventFilter(obj, event);
}
