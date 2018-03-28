#include "App.h"
#include "MainWindow.h"
#include "Settings.h"
#include "SettingsWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ConfigEditor;

App::App(int& argc, char** argv) : Super("ConfigEditor", argc, argv), mainWindow_(0), settings_(0), settingsWindow_(0)
{
    setVisibleWindowMenuNew(false);
    makeToolWindows();
}

App::~App()
{
    delete settings_;
    delete settingsWindow_;
}

void
App::makeToolWindows()
{
    settingsWindow_ = new SettingsWindow(Qt::CTRL + Qt::Key_1);
    addToolWindow(0, settingsWindow_);
    settings_ = new Settings(*settingsWindow_);
}

void
App::applicationQuit()
{
    Super::applicationQuit();
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    return new MainWindow;
}
