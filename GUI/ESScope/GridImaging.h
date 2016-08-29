#ifndef SIDECAR_GUI_ESSCOPE_GRIDIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_GRIDIMAGING_H

#include "GUI/ChannelImaging.h"
#include "GUI/IntSetting.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

/** Extension of the ChannelImaging class for the display of grids
 */
class GridImaging : public ChannelImaging
{
    Q_OBJECT
    using Super = ChannelImaging;
public:

    GridImaging(BoolSetting* enabled, ColorButtonSetting* color,
                DoubleSetting* lineWidth, OpacitySetting* opacity,
                IntSetting* alphaMajor, IntSetting* alphaMinor,
                IntSetting* betaMajor, IntSetting* betaMinor,
                IntSetting* rangeMajor, IntSetting* rangeMinor);

    int getAlphaMajor() const { return alphaMajor_->getValue(); }

    int getAlphaMinor() const { return alphaMinor_->getValue(); }

    int getBetaMajor() const { return betaMajor_->getValue(); }

    int getBetaMinor() const { return betaMinor_->getValue(); }

    int getRangeMajor() const { return rangeMajor_->getValue(); }

    int getRangeMinor() const { return rangeMinor_->getValue(); }

private:
    IntSetting* alphaMajor_;
    IntSetting* alphaMinor_;
    IntSetting* betaMajor_;
    IntSetting* betaMinor_;
    IntSetting* rangeMajor_;
    IntSetting* rangeMinor_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
