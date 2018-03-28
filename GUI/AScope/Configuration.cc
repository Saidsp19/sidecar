#include "QtGui/QDoubleSpinBox"
#include "QtGui/QSpinBox"

#include "GUI/QCheckBoxSetting.h"
#include "GUI/QDoubleSpinBoxSetting.h"
#include "GUI/QGroupBoxSetting.h"
#include "GUI/QRadioButtonSetting.h"
#include "GUI/QSpinBoxSetting.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "DefaultViewSettings.h"
#include "HistorySettings.h"
#include "PeakBarSettings.h"

using namespace SideCar::GUI::AScope;

Configuration::Configuration(ConfigurationWindow* win) :
    Super("Configuration"),
    defaultViewSettings_(new DefaultViewSettings(
        new QSpinBoxSetting(this, win->sampleMin_), new QSpinBoxSetting(this, win->sampleMax_),
        new QDoubleSpinBoxSetting(this, win->voltageMin_), new QDoubleSpinBoxSetting(this, win->voltageMax_),
        new QCheckBoxSetting(this, win->showPeakBars_))),
    historySettings_(new HistorySettings(new QCheckBoxSetting(this, win->historyEnabled_),
                                         new QSpinBoxSetting(this, win->historyDuration_))),
    peakBarSettings_(new PeakBarSettings(
        new QCheckBoxSetting(this, win->peakBarsEnabled_), new QSpinBoxSetting(this, win->peakBarsWidth_),
        new QSpinBoxSetting(this, win->peakBarsLifeTime_), new QCheckBoxSetting(this, win->peakBarsFade_)))
{
    App* app = App::GetApp();
    BoolSetting* setting;

    setting = new QRadioButtonSetting(this, win->distanceUnitsKm_, true);
    if (setting->getValue()) app->setDistanceUnits("km");

    setting = new QRadioButtonSetting(this, win->distanceUnitsNm_, true);
    if (setting->getValue()) app->setDistanceUnits("nm");

    setting = new QRadioButtonSetting(this, win->angleFormatMinSec_, true);
    if (setting->getValue()) app->setAngleFormatting(AppBase::kDegreesMinutesSeconds);

    setting = new QRadioButtonSetting(this, win->angleFormatDecimal_, true);
    if (setting->getValue()) app->setAngleFormatting(AppBase::kDecimal);

    restoreAllPresets();
}
