#include "App.h"
#include "Configuration.h"
#include "OffscreenBuffer.h"
#include "ViewSettings.h"

using namespace SideCar::GUI::ESScope;

OffscreenBuffer::OffscreenBuffer(const SampleImaging* imaging,
                                 XYWidget* view, int width,
                                 int height, int textureType)
    : Super(imaging, view->getXMin(), view->getXMax(), view->getYMin(),
            view->getYMax(), width, height, textureType)
{
    ;
}
