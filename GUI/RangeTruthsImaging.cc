#include "RangeTruthsImaging.h"

using namespace SideCar::GUI;

RangeTruthsImaging::RangeTruthsImaging(PlotPositionFunctor* plotPositionFunctor, BoolSetting* visible,
                                       ColorButtonSetting* color, DoubleSetting* extent, OpacitySetting* opacity,
                                       QComboBoxSetting* symbolType, DoubleSetting* lineWidth, IntSetting* lifeTime,
                                       BoolSetting* fadeEnabled, BoolSetting* showTrails, IntSetting* trailSize,
                                       IntSetting* tagSize, BoolSetting* showTags) :
    Super(plotPositionFunctor, visible, color, extent, opacity, symbolType, lineWidth, lifeTime, fadeEnabled,
          showTrails, tagSize, showTags),
    trailSize_(trailSize)
{
    add(trailSize);

    connect(trailSize, SIGNAL(valueChanged(int)), SIGNAL(trailSizeChanged(int)));
}
