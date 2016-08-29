#ifndef SIDECAR_GUI_PPIDISPLAY_DECAYSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_DECAYSETTINGS_H

#include "GUI/IntSetting.h"
#include "GUI/OnOffSettingsBlock.h"
#include "GUI/QComboBoxSetting.h"

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

/** Collection of Setting objects that relate to the simulated phosphor decay effect available in the PPIWidget.
 */
class DecaySettings : public OnOffSettingsBlock
{
    using Super = OnOffSettingsBlock;
public:

    DecaySettings(BoolSetting* enabled, QComboBoxSetting* profile,
                  IntSetting* distance);

    int getProfile() const { return profile_->getValue(); }

    int getDistance() const { return distance_->getValue(); }

private:
    QComboBoxSetting* profile_;
    IntSetting* distance_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
