#include "QtGui/QMessageBox"

#include "App.h"
#include "MainWindow.h"
#include "Settings.h"

#include "ConfigurationWindow.h"
#include "LogAlertsWindow.h"
#include "MainWindow.h"
#include "UtilsWidget.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

App::App(int& argc, char** argv)
    : Super("Master", argc, argv), configurationWindow_(0),
      workingDirectory_(QDir::current()), mainWindow_(0),
      logAlertsWindow_(0), settings_(0), recording_(false)
{
    setVisibleWindowMenuNew(false);
    makeToolWindows();
}

App::~App()
{
    delete settings_;
}

void
App::makeToolWindows()
{
    configurationWindow_ = new ConfigurationWindow(
	Qt::CTRL + Qt::Key_1 + kShowConfigurationWindow);
    addToolWindow(kShowConfigurationWindow, configurationWindow_);
    settings_ = new Settings(*configurationWindow_);
}

void
App::restoreWindows()
{
    Super::restoreWindows();
    if (! logAlertsWindow_)
	makeAndInitializeNewMainWindow("LogAlertsWindow");
    UtilsWidget* utilsWidget = mainWindow_->getUtilsWidget();
    connect(utilsWidget->showAlerts_, SIGNAL(clicked()),
            logAlertsWindow_, SLOT(showAndRaise()));
    connect(this, SIGNAL(closingAuxillaryWindows()),
            logAlertsWindow_, SLOT(close()));
}

void
App::applicationQuit()
{
    if (recording_) {
	if (mainWindow_->getNumActiveMasters() == 1) {
	    if (QMessageBox::question(
                    activeWindow(), "Recording Active",
                    "<p>A recording is in progress. Quitting now will stop "
                    "the recording.</p> Are you sure you want to stop "
                    "recording and exit?</p>",
                    QMessageBox::No | QMessageBox::Yes,
                    QMessageBox::No) != QMessageBox::Yes) {
		return;
	    }
	    mainWindow_->stopRecording();
	}
	else {
	    if (QMessageBox::question(
                    activeWindow(), "Recording Active",
                    "<p>A recording is in progress.</p>"
                    "<p>Are you sure you want to exit?</p>",
                    QMessageBox::No | QMessageBox::Yes,
                    QMessageBox::No) != QMessageBox::Yes) {
		return;
	    }
	}
    }

    Super::applicationQuit();
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    if (objectName == "LogAlertsWindow") {
	Q_ASSERT(logAlertsWindow_ == 0);
	logAlertsWindow_ = new LogAlertsWindow;
	return logAlertsWindow_;
    }

    if (objectName == "ConfigurationEditor")
	return 0;

    if (objectName == "LogViewWindow")
	return 0;

    if (objectName == "NotesWindow")
	return 0;

    Q_ASSERT(mainWindow_ == 0);
    mainWindow_ = new MainWindow;
    return mainWindow_;
}

ConfigurationSettings&
App::getConfigurationSettings() const
{
    return settings_->getConfigurationSettings();
}

RadarSettings&
App::getRadarSettings() const
{
    return settings_->getRadarSettings();
}
