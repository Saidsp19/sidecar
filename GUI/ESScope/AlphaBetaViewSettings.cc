#include "AlphaBetaViewSettings.h"
#include "RadarSettings.h"

using namespace SideCar::GUI::ESScope;

AlphaBetaViewSettings::AlphaBetaViewSettings(QObject* parent, const RadarSettings* radarSettings) :
    Super(parent, radarSettings,
          ViewBounds(radarSettings->getAlphaMinMin(), radarSettings->getAlphaMaxMax(), radarSettings->getBetaMinMin(),
                     radarSettings->getBetaMaxMax()))
{
    ;
}

bool
AlphaBetaViewSettings::updateViewBounds(ViewBounds& viewBounds)
{
    bool changed = false;

    if (viewBounds.xMin < radarSettings_->getAlphaMinMin()) {
        viewBounds.xMin = radarSettings_->getAlphaMinMin();
        changed = true;
    }

    if (viewBounds.xMax > radarSettings_->getAlphaMaxMax()) {
        viewBounds.xMax = radarSettings_->getAlphaMaxMax();
        changed = true;
    }

    if (viewBounds.yMin < radarSettings_->getBetaMinMin()) {
        viewBounds.yMin = radarSettings_->getBetaMinMin();
        changed = true;
    }

    if (viewBounds.yMax > radarSettings_->getBetaMaxMax()) {
        viewBounds.yMax = radarSettings_->getBetaMaxMax();
        changed = true;
    }

    return changed;
}
