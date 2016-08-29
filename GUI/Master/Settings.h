#ifndef SIDECAR_GUI_MASTER_SETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_SETTINGS_H

#include "GUI/PresetManager.h"

#include "ConfigurationSettings.h"
#include "RadarSettings.h"
#include "RecordingSettings.h"

namespace SideCar {
namespace GUI {
namespace Master {

class ConfigurationWindow;

/** Settings that persist between executions of the Master application. Since most of the settings rely on
    initial values from GUI widgets, instances of this class require a MainWindow object that hosts the GUI
    widgets. Since there is only one MainWindow in the Master application, there is only one instance of the
    Settings class.
*/
class Settings : public PresetManager
{
    Q_OBJECT
    using Super = PresetManager;
public:

    Settings(ConfigurationWindow& gui);

    ConfigurationSettings& getConfigurationSettings()
	{ return configurationSettings_; }

    RadarSettings& getRadarSettings()
	{ return radarSettings_; }

    RecordingSettings& getRecordingSettings()
	{ return recordingSettings_; }

private:
    ConfigurationSettings configurationSettings_;
    RadarSettings radarSettings_;
    RecordingSettings recordingSettings_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
