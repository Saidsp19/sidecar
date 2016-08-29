#ifndef SIDECAR_GUI_BSCOPE_PLOTPOSITIONFUNCTOR_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_PLOTPOSITIONFUNCTOR_H

#include "GUI/TargetPlotImaging.h"

namespace SideCar {
namespace GUI {
namespace BScope {

class ViewSettings;

class PlotPositionFunctor : public ::SideCar::GUI::PlotPositionFunctor
{
    using Super = ::SideCar::GUI::PlotPositionFunctor;
public:

    PlotPositionFunctor(ViewSettings& viewSettings)
	: Super(), viewSettings_(viewSettings) {}

    Vertex getPosition(const TargetPlot& plot) const;

private:
    ViewSettings& viewSettings_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
