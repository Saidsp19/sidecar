#include "QtCore/QSettings"
#include "QtGui/QIcon"
#include "QtGui/QMessageBox"

#include "GUI/LogUtils.h"
#include "GUI/PresetsWindow.h"
#include "GUI/Utils.h"

#include "App.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::TestBed;

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("testbed.App");
    return log_;
}

App::App(int& argc, char** argv) : AppBase("TestBed", argc, argv)
{
    static Logger::ProcLog log("App", Log());
    LOGINFO << std::endl;
    setVisibleWindowMenuNew(false);
}

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    return new MainWindow;
}
