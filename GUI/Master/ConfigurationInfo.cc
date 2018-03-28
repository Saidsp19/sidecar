#include <errno.h>
#include <unistd.h>

#include "QtCore/QDir"
#include "QtCore/QFile"
#include "QtCore/QFileInfo"
#include "QtCore/QSettings"
#include "QtGui/QMessageBox"
#include "QtXml/QDomDocument"
#include "QtXml/QDomElement"

#include "Configuration/RunnerConfig.h"
#include "GUI/LogUtils.h"

#include "Alert.h"
#include "App.h"
#include "ConfigurationEditor.h"
#include "ConfigurationInfo.h"
#include "ConfigurationModel.h"
#include "DiskSpace.h"
#include "Launcher.h"
#include "LogViewWindowManager.h"
#include "MainWindow.h"
#include "RunnerItem.h"
#include "RunnerLog.h"
#include "UtilsWidget.h"

static int kStartupTimerDuration = 30000; // 30 seconds in milliseconds

using namespace SideCar::Configuration;
using namespace SideCar::GUI::Master;

QString ConfigurationInfo::kStatusText_[] = {"*ERR*", " ", "---", "ON", "REC"};

Logger::Log&
ConfigurationInfo::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.ConfigurationInfo");
    return log_;
}

ConfigurationInfo::ConfigurationInfo(QObject* parent, ConfigurationModel* model, const QString& path) :
    QObject(parent), model_(model), editor_(0), diskSpaceThread_(0), configurationPath_(path), loader_(),
    status_(kError), errorText_(), running_(), runningCount_(0), startupTimer_(new QTimer(this)), lastLoaded_(),
    isRecording_(false), shuttingDown_(false), recordable_(false), viewable_(true), started_(false)
{
    Logger::ProcLog log("ConfigurationInfo", Log());
    LOGINFO << "path: " << path << std::endl;
    startupTimer_->setInterval(kStartupTimerDuration);
    startupTimer_->setSingleShot(true);
    connect(startupTimer_, SIGNAL(timeout()), SLOT(startupTimerDone()));
}

ConfigurationInfo::~ConfigurationInfo()
{
    if (diskSpaceThread_) DiskSpaceMonitor::Release(diskSpaceThread_);
    delete editor_;
}

bool
ConfigurationInfo::load(bool hideErrors)
{
    Logger::ProcLog log("load", Log());
    LOGINFO << std::endl;

    lastLoaded_ = QDateTime();
    recordable_ = false;

    running_.clear();
    MainWindow* mainWindow = GetMainWindow();
    mainWindow->getUtilsWidget()->logs_->removeConfiguration(getName());

    if (!loader_.load(configurationPath_)) {
        int line;
        int column;
        switch (loader_.getLastLoadResult()) {
        case Loader::kOK: break;

        case Loader::kFailedFileOpen: return setLoadError(hideErrors, "Failed to open configuration file");

        case Loader::kFailedXMLParse:
            loader_.getParseErrorInfo(line, column);
            if (editor_) editor_->setCursorPosition(line, column);
            return setLoadError(hideErrors, QString("Invalid XML located at position %1"
                                                    " of line %2 of file %3")
                                                .arg(column)
                                                .arg(line)
                                                .arg(configurationPath_));

        case Loader::kMissingEntityNode:
            return setLoadError(hideErrors, QString("Missing entity node in file %1").arg(loader_.getParseFilePath()));

        case Loader::kMissingSidecarNode: return setLoadError(hideErrors, "Missing <sidecar> element");

        case Loader::kMissingRadarNode: return setLoadError(hideErrors, "Missing <radar> element");

        case Loader::kInvalidRadarNode: return setLoadError(hideErrors, "Invalid <radar> element");

        case Loader::kMissingDPNode: return setLoadError(hideErrors, "Missing <dp> element");

        case Loader::kMissingStreams: return setLoadError(hideErrors, "No <stream> entities defined");

        case Loader::kMissingRunners: return setLoadError(hideErrors, "No <runner> entities defined");

        case Loader::kNotLoaded: return setLoadError(hideErrors, "No configuration file loaded.");

        default: return setLoadError(hideErrors, "Unknown error!");
        }

        return false;
    }

    QString recordingsDirectory(loader_.getRecordingsDirectory());

    if (diskSpaceThread_) {
        DiskSpaceMonitor::Release(diskSpaceThread_);
        diskSpaceThread_ = 0;
    }

    diskSpaceThread_ = DiskSpaceMonitor::AddPath(recordingsDirectory);
    if (diskSpaceThread_) {
        connect(diskSpaceThread_, SIGNAL(spaceUpdate(double, QString)), SLOT(diskSpaceUpdate(double, QString)));
        double percentUsed;
        QString freeSpace;
        diskSpaceThread_->getInfo(percentUsed, freeSpace);
        diskSpaceUpdate(percentUsed, freeSpace);
    }

    // Move into the directory we want to test to cause any automounts to take effect.
    //
    QDir::setCurrent(recordingsDirectory);
    QFileInfo fileInfo(recordingsDirectory);
    if (!hideErrors) {
        if (!fileInfo.exists()) {
            Alert::ShowInfo(GetMainWindow(), "Cannot Record",
                            QString("The recording directory '%2' used by the "
                                    "configuration '%1' does not exist. You will "
                                    "not be able to record data for this "
                                    "configuration.")
                                .arg(loader_.getConfigurationName())
                                .arg(recordingsDirectory));
        } else if (!fileInfo.isWritable()) {
            Alert::ShowInfo(GetMainWindow(), "Cannot Record",
                            QString("The recording directory '%2' used by the "
                                    "configuration '%1' does not permit writing "
                                    "by your user ID. You will not be able to "
                                    "record data for this configuration.")
                                .arg(loader_.getConfigurationName())
                                .arg(recordingsDirectory));
        }
    }

    QDir::setCurrent(App::GetApp()->getWorkingDirectory().absolutePath());

    for (size_t index = 0; index < loader_.getNumRunnerConfigs(); ++index) running_.append(false);

    QSettings settings;
    settings.beginGroup("ConfigurationInfo");
    settings.beginGroup(getName());

    recordable_ = settings.value("Recordable", true).toBool();

    setStatus(kNotRunning);
    lastLoaded_ = QDateTime::currentDateTime();

    return true;
}

bool
ConfigurationInfo::startup()
{
    Logger::ProcLog log("startup", Log());
    LOGINFO << std::endl;

    // Reload the file if it has changed since the last time it was loaded.
    //
    if (!lastLoaded_.isValid() || QFileInfo(configurationPath_).lastModified() > lastLoaded_) {
        if (!load(false)) { return false; }
    }

    // Reset our running_ tags.
    //
    for (int index = 0; index < running_.size(); ++index) running_[index] = false;
    runningCount_ = 0;
    started_ = true;

    // Create a Launcher to do the actual runner spawning. When it is done, it calls our launcherDone() method
    // below.
    //
    Launcher* launcher = new Launcher(GetMainWindow(), loader_);
    connect(launcher, SIGNAL(finished(bool)), SLOT(launcherDone(bool)));
    startupTimer_->start();
    launcher->start();

    return true;
}

void
ConfigurationInfo::launcherDone(bool wasCanceled)
{
    // Finished spawning runners. Start a timer to look for status messages from the processes.
    //
    sender()->deleteLater();
    if (wasCanceled) startupTimer_->stop();
}

void
ConfigurationInfo::startupTimerDone()
{
    Logger::ProcLog log("startupTimerDone", Log());
    LOGINFO << std::endl;

    if (runningCount_ == running_.size()) {
        Alert::ShowInfo(GetMainWindow(), "Launched",
                        QString("<p>Successfully launched all remote "
                                "processes for the configuration '%2'</p>")
                            .arg(loader_.getConfigurationName()));
    } else {
        int missing = running_.size() - runningCount_;
        QString text;
        if (runningCount_ == 0) {
            started_ = false;
            text = QString("<p>Failed to receive status reports from any "
                           "remote process%1 for the configuration '%2':</p>"
                           "<ul>")
                       .arg(missing == 1 ? "" : "es")
                       .arg(loader_.getConfigurationName());
        } else {
            text = QString("<p>Failed to receive status reports from %1 "
                           "remote process%2 for the configuration '%3':</p>"
                           "<ul>")
                       .arg(missing)
                       .arg(missing == 1 ? "" : "es")
                       .arg(loader_.getConfigurationName());
        }

        QStringList hostNames;
        for (int index = 0; index < running_.size(); ++index) {
            if (!running_[index]) {
                RunnerConfig* config = loader_.getRunnerConfig(index);
                text.append("<li>");
                text.append(config->getRunnerName());
                text.append("</li>");
                GetMainWindow()->getUtilsWidget()->logs_->addLogInfo(config);
                QString hostName = config->getHostName();
                if (!hostNames.contains(hostName)) hostNames.append(hostName);
            }
        }

        text.append("</ul><p>Please verify network / SSH connectivity to "
                    "the hosts listed below:</p><ul><li>");
        text.append(hostNames.join(QString("</li><li>")));
        text.append("</li></ul><p>Make sure the host names listed are valid, "
                    "and if not edit the configuration file to correct them."
                    "</p>");

        Alert::ShowCritical(GetMainWindow(), "Launch Failure", text);
        model_->statusChanged(this);
    }
}

void
ConfigurationInfo::foundRunner(const RunnerItem* runner)
{
    static Logger::ProcLog log("foundRunner", Log());
    QString serviceName = runner->getServiceName();
    LOGINFO << "serviceName: " << serviceName << " running_.size(): " << running_.size() << std::endl;

    for (int index = 0; index < running_.size(); ++index) {
        LOGDEBUG << index << " serviceName: " << loader_.getRunnerConfig(index)->getServiceName() << std::endl;
        const RunnerConfig* runnerConfig = loader_.getRunnerConfig(index);
        if (serviceName == runnerConfig->getServiceName()) {
            LOGDEBUG << "matched" << std::endl;
            if (running_[index] == false) {
                running_[index] = true;
                ++runningCount_;
                checkRunningCount();
                LOGDEBUG << "runningCount: " << runningCount_ << std::endl;
                break;
            }
        }
    }

    if (runner->isRecording()) setRecordingState(true);
}

void
ConfigurationInfo::lostRunner(const QString& serviceName)
{
    for (int index = 0; index < running_.size(); ++index) {
        const RunnerConfig* runnerConfig = loader_.getRunnerConfig(index);
        if (serviceName == runnerConfig->getServiceName()) {
            if (running_[index] == true) {
                running_[index] = false;
                --runningCount_;
                checkRunningCount();
                break;
            }
        }
    }
}

void
ConfigurationInfo::checkRunningCount()
{
    Logger::ProcLog log("checkRunningCount", Log());
    LOGINFO << getName() << " runningCount: " << runningCount_ << std::endl;

    Status newStatus = status_;
    if (runningCount_ == 0) {
        if (!startupTimer_->isActive()) newStatus = kNotRunning;
    } else if (runningCount_ < running_.size()) {
        newStatus = kPartial;
        if (started_ && !shuttingDown_ && isRunning() && getStatus() != newStatus) {
            QTimer::singleShot(1000, this, SLOT(partialNotify()));
        }
    } else {
        newStatus = kRunning;
        if (startupTimer_->isActive()) {
            startupTimer_->stop();
            startupTimerDone();
        }
    }

    setStatus(newStatus);
}

void
ConfigurationInfo::partialNotify()
{
    if (status_ == kPartial) {
        int missing = running_.size() - runningCount_;
        QString text = QString("<p>Lost status publishing information for %1 "
                               "remote process%2 of the configuration '%3'.</p><ul>")
                           .arg(missing)
                           .arg(missing == 1 ? "" : "es")
                           .arg(loader_.getConfigurationName());

        QStringList hostNames;
        for (int index = 0; index < running_.size(); ++index) {
            if (!running_[index]) {
                QString name = loader_.getRunnerConfig(index)->getRunnerName();
                text.append("<li>");
                text.append(name);
                text.append("</li>");
                QString hostName = loader_.getRunnerConfig(index)->getHostName();
                if (!hostNames.contains(hostName)) hostNames.append(hostName);
            }
        }

        text.append("</ul></p><p>Please verify network / SSH connectivity to "
                    "the host names listed below:</p><ul><li>");
        text.append(hostNames.join("</li><li>"));
        text.append(QString("</li></ul><p>Shutdown configuration '%1'?").arg(loader_.getConfigurationName()));

        int rc = QMessageBox::critical(qApp->activeWindow(), "Lost Remote Processes", text,
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (rc == QMessageBox::Yes) {
            MainWindow* mainWindow = qobject_cast<MainWindow*>(GetMainWindow());
            if (mainWindow) {
                shuttingDown_ = true;
                mainWindow->forceShutdown(this);
            }
        }
    }
}

void
ConfigurationInfo::setStatus(Status status)
{
    static Logger::ProcLog log("setStatus", Log());

    LOGINFO << "current status: " << status_ << " new status: " << status << std::endl;
    if (status == kNotRunning && status_ != kError)
        GetMainWindow()->showMessage(getName() + " shut down.");
    else if (status == kRunning)
        GetMainWindow()->showMessage(getName() + " running.");

    status_ = status;
    model_->statusChanged(this);

    if (status == kNotRunning) {
        started_ = false;
        shuttingDown_ = false;
    }
}

void
ConfigurationInfo::setRecordingState(bool state)
{
    if (state != isRecording_) {
        isRecording_ = state;
        model_->statusChanged(this);
    }
}

MainWindow*
ConfigurationInfo::GetMainWindow()
{
    return App::GetApp()->getMainWindow();
}

bool
ConfigurationInfo::hasValidRecordingDirectory() const
{
    QFileInfo recordingDirBaseFileInfo(loader_.getRecordingsDirectory());
    return recordingDirBaseFileInfo.exists() && recordingDirBaseFileInfo.isWritable();
}

bool
ConfigurationInfo::setLoadError(bool noPrompt, const QString& text)
{
    Logger::ProcLog log("setLoadError", Log());
    LOGERROR << noPrompt << ' ' << text << std::endl;
    if (!noPrompt) Alert::ShowWarning(App::GetApp()->activeWindow(), "Load Error", text);
    errorText_ = text;
    setStatus(kError);
    return false;
}

bool
ConfigurationInfo::ShowError(const QString& title, const QString& text)
{
    Logger::ProcLog log("ShowError", Log());
    LOGERROR << text << std::endl;
    Alert::ShowWarning(GetMainWindow(), title, text);
    return false;
}

void
ConfigurationInfo::diskSpaceUpdate(double percentUsed, QString freeSpace)
{
    if (percentUsed != recordingDirPercentUsed_ || freeSpace != recordingDirFreeSpace_) {
        recordingDirPercentUsed_ = percentUsed;
        recordingDirFreeSpace_ = freeSpace;
        model_->spaceAvailableChanged(this);
    }
}

void
ConfigurationInfo::setViewable(bool state)
{
    if (state != viewable_) {
        viewable_ = state;
        QSettings settings;
        settings.beginGroup("ConfigurationInfo");
        settings.beginGroup(getName());
        settings.setValue("Viewable", state);
    }
}

void
ConfigurationInfo::setRecordable(bool state)
{
    if (state != recordable_) {
        recordable_ = state;
        QSettings settings;
        settings.beginGroup("ConfigurationInfo");
        settings.beginGroup(getName());
        settings.setValue("Recordable", state);
    }
}

QString
ConfigurationInfo::getStatusText() const
{
    QString text;
    if (status_ == kPartial) {
        text = QString("%1 of %2").arg(runningCount_).arg(running_.size());
        if (isRecording_) text.append("*");
    } else {
        text = kStatusText_[getStatus()];
    }

    if (shuttingDown_) text += '-';

    if (startupTimer_->isActive()) text += '+';

    return text;
}

ConfigurationEditor*
ConfigurationInfo::getEditor()
{
    if (!editor_) {
        editor_ = new ConfigurationEditor(*this);
        editor_->initialize();
    }

    return editor_;
}
