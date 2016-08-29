#include "QtGui/QMessageBox"

#include "GUI/LogUtils.h"

#include "App.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ZeroconfMonitor;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("zcm.App");
    return log_;
}

App::App(int& argc, char** argv)
    : AppBase("zcm", argc, argv)
{
    Logger::ProcLog log("App", Log());
    LOGINFO << std::endl;
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    Logger::ProcLog log("makeNewMainWindow", Log());
    LOGINFO << "objectName: " << objectName << std::endl;
    return new MainWindow;
}


void
App::showAbout()
{
    QMessageBox::about(qApp->activeWindow(), "ZeroconfMonitor",
                       "<p>Zeroconf Monitor (zcm)</p>"
                       "<p>Shows active SideCar subscribers, publishers, and "
                       "status emitters. Useful in debugging connectivity "
                       "problems."
                       "<p>Version 1.0</p>"
                       "<p>Author: Brad Howes, Group 42</p>");
}
