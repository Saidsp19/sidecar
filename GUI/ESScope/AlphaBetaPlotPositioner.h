#ifndef SIDECAR_GUI_ESSCOPE_ALPHABETAPLOTPOSITIONER_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_ALPHABETAPLOTPOSITIONER_H

#include "GUI/TargetPlotImaging.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class RadarSettings;

class AlphaBetaPlotPositioner : public PlotPositionFunctor
{
    using Super = PlotPositionFunctor;
public:

    AlphaBetaPlotPositioner();

    Vertex getPosition(const TargetPlot& plot) const;

private:
    RadarSettings* radarSettings_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
