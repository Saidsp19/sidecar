#ifndef SIDECAR_GUI_PPIDISPLAY_BINARYVERTEXGENERATOR_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_BINARYVERTEXGENERATOR_H

#include "GUI/VertexGenerator.h"

namespace Logger {
class Log;
}
namespace Utils {
class SineCosineLUT;
}

namespace SideCar {
namespace GUI {

class SampleImaging;

namespace PPIDisplay {

class ViewSettings;

/** Implementation of the VertexGenerator abstract base class that knows how to generate Vertex and Color values
    from SideCar BinaryVideo messages.
*/
class BinaryVertexGenerator : public VertexGenerator {
    using Super = VertexGenerator;

public:
    static Logger::Log& Log();

    BinaryVertexGenerator();

private:
    void renderMessage(const Messages::PRIMessage::Ref& msg, VertexColorArray& points);

    SampleImaging* imaging_;
    const Utils::SineCosineLUT* sineCosineLUT_;
    ViewSettings* viewSettings_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
