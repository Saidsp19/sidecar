#include "OffscreenBuffer.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::BScope;

OffscreenBuffer::OffscreenBuffer(const SampleImaging* imaging, ViewSettings* viewSettings, int width, int height,
                                 int textureType) :
    Super(imaging, viewSettings->getAzimuthMin(), viewSettings->getAzimuthMax(), viewSettings->getRangeMin(),
          viewSettings->getRangeMax(), width, height, textureType)
{
    connect(viewSettings, SIGNAL(settingChanged()), SLOT(viewSettingsChanged()));
}

void
OffscreenBuffer::viewSettingsChanged()
{
    ViewSettings* viewSettings = qobject_cast<ViewSettings*>(sender());
    setBounds(viewSettings->getAzimuthMin(), viewSettings->getAzimuthMax(), viewSettings->getRangeMin(),
              viewSettings->getRangeMax());
}
