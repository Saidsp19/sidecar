#include "App.h"
#include "BrowserWindow.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Playback;

App::App(int& argc, char** argv) : Super("Playback", argc, argv)
{
    browserWindow_ = new BrowserWindow(Qt::CTRL + Qt::SHIFT + Qt::Key_1);
    addToolWindow(kShowBrowserWindow, browserWindow_);
    setVisibleWindowMenuNew(false);
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    return new MainWindow;
}
