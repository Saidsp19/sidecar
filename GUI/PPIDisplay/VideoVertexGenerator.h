#ifndef SIDECAR_GUI_PPIDISPLAY_VIDEOVERTEXGENERATOR_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_VIDEOVERTEXGENERATOR_H

#include "GUI/VertexGenerator.h"

namespace Utils { class SineCosineLUT; }

namespace SideCar {
namespace GUI {

class VideoImaging;
class VideoSampleCountTransform;

namespace PPIDisplay {

class VideoImaging;
class ViewSettings;

/** Implementation of the VertexGenerator abstract base class that knows how to generate Vertex and Color values
    from SideCar Video messages.
*/
class VideoVertexGenerator : public VertexGenerator
{
    using Super = VertexGenerator;
public:

    VideoVertexGenerator();

private:

    void renderMessage(const Messages::PRIMessage::Ref& msg, VertexColorArray& points);

    VideoImaging* imaging_;
    VideoSampleCountTransform* transform_;
    const Utils::SineCosineLUT* sineCosineLUT_;
    ViewSettings* viewSettings_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
