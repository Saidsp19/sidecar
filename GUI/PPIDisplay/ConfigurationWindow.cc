#include "QtCore/QSettings"

#include "GUI/CLUT.h"
#include "GUI/LogUtils.h"
#include "GUI/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "ViewSettings.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::PPIDisplay;

Logger::Log&
ConfigurationWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.ConfigurationWindow");
    return log_;
}

ConfigurationWindow::ConfigurationWindow(int shortcut) :
    ToolWindowBase("ConfigurationWindow", "Settings", shortcut), Ui::ConfigurationWindow()
{
    Logger::ProcLog log("ConfigurationWindow", Log());
    LOGINFO << std::endl;
    setupUi(this);
    setFixedSize();

    QString degreeSuffix(GUI::DegreeSymbol());
    decayDistance_->setSuffix(degreeSuffix);
    rangeRingsAzimuthMajor_->setSuffix(degreeSuffix);

    updateDistanceUnits(QString(' ') + getApp()->getDistanceUnits());
    connect(getApp(), SIGNAL(distanceUnitsChanged(const QString&)), SLOT(updateDistanceUnits(const QString&)));

    CLUT::AddTypeNames(videoColorMap_);
}

void
ConfigurationWindow::setConfiguration(Configuration* configuration)
{
    connect(configuration->getViewSettings(), SIGNAL(rangeMaxMaxChanged(double)), this,
            SLOT(rangeMaxMaxChanged(double)));
}

void
ConfigurationWindow::rangeMaxMaxChanged(double value)
{
    rangeMax_->setMaximum(value);
    rangeMaxMax_->setText(QString("(max %1 %2)").arg(value, 0, 'f', 2).arg(getApp()->getDistanceUnits()));
}

void
ConfigurationWindow::on_distanceUnitsKm__toggled(bool enabled)
{
    if (enabled) { getApp()->setDistanceUnits("km"); }
}

void
ConfigurationWindow::on_distanceUnitsNm__toggled(bool enabled)
{
    if (enabled) { getApp()->setDistanceUnits("nm"); }
}

void
ConfigurationWindow::on_angleFormatMinSec__toggled(bool enabled)
{
    if (enabled) { getApp()->setAngleFormatting(AppBase::kDegreesMinutesSeconds); }
}

void
ConfigurationWindow::on_angleFormatDecimal__toggled(bool enabled)
{
    if (enabled) { getApp()->setAngleFormatting(AppBase::kDecimal); }
}

void
ConfigurationWindow::updateDistanceUnits(const QString& suffix)
{
    rangeMax_->setSuffix(suffix);
    rangeRingsRangeMajor_->setSuffix(suffix);
    rangeMaxMaxChanged(rangeMax_->maximum());
}

void
ConfigurationWindow::on_videoColorMapEnabled__toggled(bool enabled)
{
    videoColor_->setEnabled(!enabled);
}

void
ConfigurationWindow::on_videoColorMap__currentIndexChanged(int index)
{
    videoDesaturateEnabled_->setEnabled(CLUT::HasSaturation(CLUT::Type(index)));
}
