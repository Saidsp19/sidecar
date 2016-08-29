#include "QtCore/QSettings"

#include "GUI/CLUT.h"
#include "GUI/LogUtils.h"
#include "GUI/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;

Logger::Log&
ConfigurationWindow::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("bscope.ConfigurationWindow");
    return log_;
}

ConfigurationWindow::ConfigurationWindow(int shortcut)
    : ToolWindowBase("ConfigurationWindow", "Settings", shortcut),
      Ui::ConfigurationWindow()
{
    setupUi(this);
    setFixedSize();

    QString degreeSuffix(GUI::DegreeSymbol());
    azimuthSpan_->setSuffix(degreeSuffix);
    rangeRingsAzimuthMajor_->setSuffix(degreeSuffix);

    updateDistanceUnits(QString(' ') + getApp()->getDistanceUnits());
    connect(getApp(), SIGNAL(distanceUnitsChanged(const QString&)),
            SLOT(updateDistanceUnits(const QString&)));

    CLUT::AddTypeNames(videoColorMap_);
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

void
ConfigurationWindow::updateDistanceUnits(const QString& units)
{
    rangeMin_->setSuffix(units);
    rangeMax_->setSuffix(units);
    rangeRingsRangeMajor_->setSuffix(units);
}

void
ConfigurationWindow::on_videoColorMapEnabled__toggled(bool enabled)
{
    videoColor_->setEnabled(! enabled);
}

void
ConfigurationWindow::on_videoColorMap__currentIndexChanged(int index)
{
    videoDesaturateEnabled_->setEnabled(
	CLUT::HasSaturation(CLUT::Type(index)));
}
