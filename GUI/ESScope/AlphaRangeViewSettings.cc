#include "AlphaRangeViewSettings.h"
#include "RadarSettings.h"

using namespace SideCar::GUI::ESScope;

AlphaRangeViewSettings::AlphaRangeViewSettings(QObject* parent, const RadarSettings* radarSettings) :
    Super(parent, radarSettings,
          ViewBounds(radarSettings->getAlphaMinMin(), radarSettings->getAlphaMaxMax(), radarSettings->getRangeMinMin(),
                     radarSettings->getRangeMaxMax()))
{
    ;
}

bool
AlphaRangeViewSettings::updateViewBounds(ViewBounds& viewBounds)
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

    if (viewBounds.yMin < radarSettings_->getRangeMinMin()) {
        viewBounds.yMin = radarSettings_->getRangeMinMin();
        changed = true;
    }

    if (viewBounds.yMax > radarSettings_->getRangeMaxMax()) {
        viewBounds.yMax = radarSettings_->getRangeMaxMax();
        changed = true;
    }

    return changed;
}
