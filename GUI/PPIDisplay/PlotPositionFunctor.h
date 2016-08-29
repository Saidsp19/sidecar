#ifndef SIDECAR_GUI_PPIDISPLAY_PLOTPOSITIONFUNCTOR_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_PLOTPOSITIONFUNCTOR_H

#include "GUI/TargetPlotImaging.h"

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

/** Implementation of the GUI::PlotPositionFunctor interface. Returns the X, Y positions for TargetPlot objects.
 */
class PlotPositionFunctor : public ::SideCar::GUI::PlotPositionFunctor
{
    using Super = ::SideCar::GUI::PlotPositionFunctor;
public:

    /** Obtain the plot position for a given TargetPlot object

        \param plot the TargetPlot to work with

        \return TargetPlot x, y location
    */
    Vertex getPosition(const TargetPlot& plot) const;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
