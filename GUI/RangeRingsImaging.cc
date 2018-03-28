#include "RangeRingsImaging.h"

using namespace SideCar::GUI;

RangeRingsImaging::RangeRingsImaging(BoolSetting* enabled, ColorButtonSetting* color, DoubleSetting* lineWidth,
                                     OpacitySetting* opacity, IntSetting* azimuthSpacing, IntSetting* azimuthTicks,
                                     DoubleSetting* rangeSpacing, IntSetting* rangeTicks) :
    Super(enabled, color, lineWidth, opacity),
    azimuthSpacing_(azimuthSpacing), azimuthTicks_(azimuthTicks), rangeSpacing_(rangeSpacing), rangeTicks_(rangeTicks)
{
    add(azimuthSpacing);
    add(azimuthTicks);
    add(rangeSpacing);
    add(rangeTicks);
    connect(azimuthSpacing, SIGNAL(valueChanged(int)), SIGNAL(tickSettingsChanged()));
    connect(azimuthTicks, SIGNAL(valueChanged(int)), SIGNAL(tickSettingsChanged()));
    connect(rangeSpacing, SIGNAL(valueChanged(double)), SIGNAL(tickSettingsChanged()));
    connect(rangeTicks, SIGNAL(valueChanged(int)), SIGNAL(tickSettingsChanged()));
}
