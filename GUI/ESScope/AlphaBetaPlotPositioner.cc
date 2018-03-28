#include "AlphaBetaPlotPositioner.h"
#include "App.h"
#include "Configuration.h"
#include "RadarSettings.h"

using namespace SideCar::GUI::ESScope;

AlphaBetaPlotPositioner::AlphaBetaPlotPositioner() :
    Super(), radarSettings_(App::GetApp()->getConfiguration()->getRadarSettings())
{
    ;
}

SideCar::GUI::Vertex
AlphaBetaPlotPositioner::getPosition(const SideCar::GUI::TargetPlot& plot) const
{
    double alpha, beta;
    radarSettings_->getAlphaBeta(plot.getAzimuth(), plot.getElevation(), &alpha, &beta);
    return Vertex(alpha, beta);
}
