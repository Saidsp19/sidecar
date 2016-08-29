#include "Messages/Video.h"

#include "App.h"
#include "Configuration.h"
#include "VideoImaging.h"
#include "VideoOffscreenBuffer.h"
#include "VideoSampleCountTransform.h"

using namespace SideCar;
using namespace SideCar::GUI::PPIDisplay;

VideoOffscreenBuffer::VideoOffscreenBuffer(double rangeMax, int width,
                                           int height, int textureType)
    : Super(App::GetApp()->getConfiguration()->getVideoImaging(), rangeMax,
            width, height, textureType)
{
    ;
}
