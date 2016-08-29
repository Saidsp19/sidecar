#include "QtCore/QTimer"

#include "GUI/LogUtils.h"
#include "GUI/PresetManager.h"

#include "App.h"
#include "ConfigurationWindow.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::HealthAndStatus;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("hands.App");
    return log_;
}

App::App(int& argc, char** argv)
    : AppBase("HealthAndStatus", argc, argv),
      presetManager_(new PresetManager("HealthAndStatus", this)),
      configurationWindow_(0), mainWindow_(0)
{
    static Logger::ProcLog log("App", Log());
    LOGINFO << std::endl;

    setVisibleWindowMenuNew(false);
    configurationWindow_ = new ConfigurationWindow(
	Qt::CTRL + Qt::Key_1 + kShowConfigurationWindow);
    configurationWindow_->useDefaultSettings();

    addToolWindow(kShowConfigurationWindow, configurationWindow_);
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    static Logger::ProcLog log("makeNewMainWindow", Log());
    LOGINFO << "objectName: " << objectName << std::endl;
    mainWindow_ = new MainWindow;
    return mainWindow_;
}

void
App::applicationQuit()
{
    presetManager_->saveAllPresets();
    Super::applicationQuit();
}
