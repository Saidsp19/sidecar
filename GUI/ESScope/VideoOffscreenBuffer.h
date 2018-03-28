#ifndef SIDECAR_GUI_ESSCOPE_VIDEOOFFSCREENBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_VIDEOOFFSCREENBUFFER_H

#include "Messages/Video.h"

#include "GUI/OffscreenBuffer.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class VideoSampleCountTransform;

namespace ESScope {

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
    static Logger::Log& Log();

    /** Constructor.

        \param sineCosineLUT

        \param points

        \param imaging

        \param visibleRangeMax

        \param transform
    */
    VideoOffscreenBuffer(History* history);

    void addPoints(int index);

    /** Obtain the imaging configuration for video data.

        \return VideoImaging reference
    */
    const VideoImaging* getImaging() const;

private:
    const VideoSampleCountTransform* transform_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
