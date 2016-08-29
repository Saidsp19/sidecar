#ifndef SIDECAR_GUI_RANGETRUTHSIMAGING_H // -*- C++ -*-
#define SIDECAR_GUI_RANGETRUTHSIMAGING_H

#include "GUI/TargetPlotImaging.h"

namespace SideCar {
namespace GUI {

class RangeTruthsImaging : public TargetPlotImaging
{
    Q_OBJECT
    using Super = TargetPlotImaging;
public:

    RangeTruthsImaging(PlotPositionFunctor* plotPositionFunctor,
                       BoolSetting* visible, ColorButtonSetting* color,
                       DoubleSetting* extent, OpacitySetting* opacity,
                       QComboBoxSetting* symbolType, DoubleSetting* lineWidth,
                       IntSetting* lifeTime, BoolSetting* fadeEnabled,
                       BoolSetting* showTrails, IntSetting* trailSize,
                       IntSetting* tagSize, BoolSetting* showTags);

    int getTrailSize() const { return trailSize_->getValue(); }

signals:

    void trailSizeChanged(int value);

private:
    IntSetting* trailSize_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
