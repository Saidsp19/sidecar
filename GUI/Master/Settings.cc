#include "GUI/QCheckBoxSetting.h"
#include "GUI/QDoubleSpinBoxSetting.h"
#include "GUI/QLineEditSetting.h"
#include "GUI/TimeSetting.h"

#include "App.h"
#include "ConfigurationWindow.h"
#include "Settings.h"

using namespace SideCar::GUI::Master;

Settings::Settings(ConfigurationWindow& win) :
    Super("Configuration"), configurationSettings_(new StringListSetting(this, "ConfigPaths")),
    radarSettings_(new QCheckBoxSetting(this, win.radarTransmitting_),
                   new QDoubleSpinBoxSetting(this, win.radarFrequency_), new QCheckBoxSetting(this, win.radarRotating_),
                   new QDoubleSpinBoxSetting(this, win.radarRotation_), new QCheckBoxSetting(this, win.drfmOn_),
                   new QLineEditSetting(this, win.drfmConfig_)),
    recordingSettings_(new BoolSetting(this, "recordingDurationEnable_", true, false),
                       new TimeSetting(this, "recordingDuration_", QTime(0, 1, 0)))
{
    restoreAllPresets();
}
