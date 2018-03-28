#include "GUI/DoubleMinMaxValidator.h"
#include "GUI/IntMinMaxValidator.h"
#include "GUI/LogUtils.h"

#include "App.h"
#include "ConfigurationWindow.h"

using namespace SideCar::GUI::Master;

Logger::Log&
ConfigurationWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.ConfigurationWindow");
    return log_;
}

ConfigurationWindow::ConfigurationWindow(int shortcut) :
    ToolWindowBase("ConfigurationWindow", "Settings", shortcut), Ui::ConfigurationWindow()
{
    setupUi(this);
    setFixedSize();
}
