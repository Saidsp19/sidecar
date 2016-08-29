#ifndef SIDECAR_GUI_RANGERINGSIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_RANGERINGSIMAGING_H

#include "GUI/ChannelImaging.h"
#include "GUI/IntSetting.h"

namespace SideCar {
namespace GUI {

/** Extension of the ChannelImaging class for the display of range rings.
 */
class RangeRingsImaging : public ChannelImaging
{
    Q_OBJECT
    using Super = ChannelImaging;
public:

    RangeRingsImaging(BoolSetting* enabled, ColorButtonSetting* color,
                      DoubleSetting* lineWidth, OpacitySetting* opacity,
                      IntSetting* azimuthSpacing, IntSetting* azimuthTicks,
                      DoubleSetting* rangeSpacing, IntSetting* rangeTicks);

    int getAzimuthSpacing() const { return azimuthSpacing_->getValue(); }

    int getAzimuthTicks() const { return azimuthTicks_->getValue(); }

    float getRangeSpacing() const { return rangeSpacing_->getValue(); }

    int getRangeTicks() const { return rangeTicks_->getValue(); }

signals:

    void tickSettingsChanged();

private:
    IntSetting* azimuthSpacing_;
    IntSetting* azimuthTicks_;
    DoubleSetting* rangeSpacing_;
    IntSetting* rangeTicks_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
