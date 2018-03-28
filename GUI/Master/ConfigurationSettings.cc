#include "QtCore/QSettings"

#include "ConfigurationSettings.h"

using namespace SideCar::GUI::Master;

ConfigurationSettings::ConfigurationSettings(StringListSetting* configPaths) : Super(), configPaths_(configPaths)
{
    add(configPaths);
}
