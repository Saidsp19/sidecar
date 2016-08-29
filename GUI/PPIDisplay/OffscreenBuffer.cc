#include "App.h"
#include "Configuration.h"
#include "OffscreenBuffer.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::PPIDisplay;

OffscreenBuffer::OffscreenBuffer(const SampleImaging* imaging, double rangeMax,
                                 int width, int height, int textureType)
    : Super(imaging, -rangeMax, rangeMax, -rangeMax, rangeMax, width, height,
            textureType)
{
    App* app = App::GetApp();
    Configuration* configuration = app->getConfiguration();
    ViewSettings* viewSettings = configuration->getViewSettings();
    connect(viewSettings, SIGNAL(rangeMaxChanged(double)),
            SLOT(rangeMaxChanged(double)));
}

void
OffscreenBuffer::rangeMaxChanged(double rangeMax)
{
    if (isValid())
	setBounds(-rangeMax, rangeMax, -rangeMax, rangeMax);
}
