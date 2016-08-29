#include "DecaySettings.h"

using namespace SideCar::GUI::PPIDisplay;

DecaySettings::DecaySettings(BoolSetting* enabled, QComboBoxSetting* profile,
                             IntSetting* distance)
    : OnOffSettingsBlock(enabled), profile_(profile), distance_(distance)
{
    add(profile);
    add(distance);
}
