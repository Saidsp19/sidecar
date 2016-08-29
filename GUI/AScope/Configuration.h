#ifndef SIDECAR_GUI_ASCOPE_CONFIGURATION_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_CONFIGURATION_H

#include "GUI/PresetManager.h"

namespace SideCar {
namespace GUI {

class BoolSetting;
class DoubleSetting;

namespace AScope {

class ConfigurationWindow;
class DefaultViewSettings;
class HistorySettings;
class PeakBarSettings;
class VoltageScaleSettings;

class Configuration : public PresetManager
{
    Q_OBJECT
    using Super = PresetManager;
public:

    Configuration(ConfigurationWindow* gui);

    DefaultViewSettings& getDefaultViewSettings() const
	{ return *defaultViewSettings_; }

    HistorySettings& getHistorySettings() const { return *historySettings_; }

    PeakBarSettings& getPeakBarSettings() const { return *peakBarSettings_; }

private:
    DefaultViewSettings* defaultViewSettings_;
    HistorySettings* historySettings_;
    PeakBarSettings* peakBarSettings_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

#endif
