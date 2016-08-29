#include "PlotPositionFunctor.h"

using namespace SideCar::GUI::PPIDisplay;

SideCar::GUI::Vertex
PlotPositionFunctor::getPosition(const SideCar::GUI::TargetPlot& plot) const
{
    return Vertex(plot.getX(), plot.getY());
}
