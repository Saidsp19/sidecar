#include "GUI/CLUT.h"
#include "GUI/Utils.h"

#include "App.h"
#include "ConfigurationWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

ConfigurationWindow::ConfigurationWindow(int shortcut)
    : ToolWindowBase("ConfigurationWindow", "Settings", shortcut),
      Ui::ConfigurationWindow()
{
    setupUi(this);
    setFixedSize();
    QString degreeSuffix(GUI::DegreeSymbol());
    faceTilt_->setSuffix(degreeSuffix);
    faceRotation_->setSuffix(degreeSuffix);
    CLUT::AddTypeNames(videoColorMap_);
}

void
ConfigurationWindow::on_distanceUnitsKm__toggled(bool enabled)
{
    if (enabled)
	getApp()->setDistanceUnits("km");
}

void
ConfigurationWindow::on_distanceUnitsNm__toggled(bool enabled)
{
    if (enabled)
	getApp()->setDistanceUnits("nm");
}

void
ConfigurationWindow::on_angleFormatMinSec__toggled(bool enabled)
{
    if (enabled)
	getApp()->setAngleFormatting(AppBase::kDegreesMinutesSeconds);
}

void
ConfigurationWindow::on_angleFormatDecimal__toggled(bool enabled)
{
    if (enabled)
	getApp()->setAngleFormatting(AppBase::kDecimal);
}
