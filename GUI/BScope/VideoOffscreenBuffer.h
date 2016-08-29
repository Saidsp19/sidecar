#ifndef SIDECAR_GUI_BSCOPE_VIDEOOFFSCREENBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_VIDEOOFFSCREENBUFFER_H

#include "Messages/Video.h"

#include "OffscreenBuffer.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class VideoSampleCountTransform;

namespace BScope {

class VideoImaging;

/** Derivation of OffscreenBuffer that understands how to generate OpenGL point data from Messages::Video sample
    data.

    Supports decimation of the incoming data. The value from
    SampleImaging::getDecimation() controls the amount of decimation that
    occurs.
*/
class VideoOffscreenBuffer : public OffscreenBuffer
{
    using Super = OffscreenBuffer;
public:
    static Logger::Log& Log();
 
    /** Constructor.
     */
    VideoOffscreenBuffer(int width, int height, int textureType = -1);

    /** Generate point data from a samples found in a Messages::Video message. Relies on
        VideoSampleCountTransform to do the sample value conversion.

        \param msg container of samples to convert
    */
    void addPoints(const Messages::PRIMessage::Ref& msg);

    /** Obtain the imaging configuration for video data.

        \return VideoImaging reference
    */
    const VideoImaging* getImaging() const;

private:

    VideoImaging* imaging_;
    VideoSampleCountTransform* transform_;    
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
