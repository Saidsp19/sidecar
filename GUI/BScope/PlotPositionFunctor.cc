#include "PlotPositionFunctor.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

SideCar::GUI::Vertex
PlotPositionFunctor::getPosition(const SideCar::GUI::TargetPlot& plot) const
{
    return Vertex(viewSettings_.normalizedAzimuth(plot.getAzimuth()), plot.getRange());
}
