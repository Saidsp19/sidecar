#include "GUI/DoubleMinMaxValidator.h"
#include "GUI/IntMinMaxValidator.h"
#include "GUI/LogUtils.h"

#include "App.h"
#include "ConfigurationWindow.h"

using namespace SideCar::GUI::AScope;

Logger::Log&
ConfigurationWindow::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("ascope.ConfigurationWindow");
    return log_;
}

ConfigurationWindow::ConfigurationWindow(int shortcut)
    : ToolWindowBase("ConfigurationWindow", "Settings", shortcut),
      Ui::ConfigurationWindow()
{
    setupUi(this);
    setFixedSize();
    new IntMinMaxValidator(this, sampleMin_, sampleMax_);
    new DoubleMinMaxValidator(this, voltageMin_, voltageMax_);
}

void
ConfigurationWindow::on_distanceUnitsKm__toggled(bool enabled)
{
    if (enabled) {
	getApp()->setDistanceUnits("km");
    }
}

void
ConfigurationWindow::on_distanceUnitsNm__toggled(bool enabled)
{
    if (enabled) {
	getApp()->setDistanceUnits("nm");
    }
}

void
ConfigurationWindow::on_angleFormatMinSec__toggled(bool enabled)
{
    if (enabled) {
	getApp()->setAngleFormatting(AppBase::kDegreesMinutesSeconds);
    }
}

void
ConfigurationWindow::on_angleFormatDecimal__toggled(bool enabled)
{
    if (enabled) {
	getApp()->setAngleFormatting(AppBase::kDecimal);
    }
}
