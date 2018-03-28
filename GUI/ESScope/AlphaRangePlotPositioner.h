#ifndef SIDECAR_GUI_ESSCOPE_ALPHARANGEPLOTPOSITIONER_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_ALPHARANGEPLOTPOSITIONER_H

#include "GUI/TargetPlotImaging.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class RadarSettings;

class AlphaRangePlotPositioner : public PlotPositionFunctor {
    using Super = PlotPositionFunctor;

public:
    AlphaRangePlotPositioner();

    Vertex getPosition(const TargetPlot& plot) const;

private:
    RadarSettings* radarSettings_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
