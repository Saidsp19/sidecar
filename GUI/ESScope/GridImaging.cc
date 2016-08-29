#include "GridImaging.h"

using namespace SideCar::GUI::ESScope;

GridImaging::GridImaging(BoolSetting* enabled, ColorButtonSetting* color,
                         DoubleSetting* lineWidth, OpacitySetting* opacity,
                         IntSetting* alphaMajor, IntSetting* alphaMinor,
                         IntSetting* betaMajor, IntSetting* betaMinor,
                         IntSetting* rangeMajor, IntSetting* rangeMinor)
    : Super(enabled, color, lineWidth, opacity), alphaMajor_(alphaMajor),
      alphaMinor_(alphaMinor), rangeMajor_(rangeMajor),
      rangeMinor_(rangeMinor)
{
    add(alphaMajor);
    add(alphaMinor);
    add(betaMajor);
    add(betaMinor);
    add(rangeMajor);
    add(rangeMinor);
}
