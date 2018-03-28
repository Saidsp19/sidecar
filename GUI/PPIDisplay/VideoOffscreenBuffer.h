#ifndef SIDECAR_GUI_PPIDISPLAY_VIDEOOFFSCREENBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_VIDEOOFFSCREENBUFFER_H

#include "OffscreenBuffer.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class VideoSampleCountTransform;

namespace PPIDisplay {

class VideoImaging;

/** Derivation of OffscreenBuffer that understands how to generate OpenGL point data from Messages::Video sample
    data.

    Supports decimation of the incoming data. The value from
    SampleImaging::getDecimation() controls the amount of decimation that
    occurs.
*/
class VideoOffscreenBuffer : public OffscreenBuffer {
    using Super = OffscreenBuffer;

public:
    VideoOffscreenBuffer(double rangeMax, int width, int height, int textureType);
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
