#ifndef SIDECAR_GUI_BSCOPE_BINARYVERTEXGENERATOR_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_BINARYVERTEXGENERATOR_H

#include "GUI/VertexGenerator.h"

namespace SideCar {
namespace GUI {

class SampleImaging;

namespace BScope {

class ViewSettings;

/** Implementation of the VertexGenerator abstract base class that knows how to generate Vertex and Color values
    from SideCar BinaryVideo messages.
*/
class BinaryVertexGenerator : public VertexGenerator {
    using Super = VertexGenerator;

public:
    BinaryVertexGenerator();

private:
    void renderMessage(const Messages::PRIMessage::Ref& msg, VertexColorArray& points);

    SampleImaging* imaging_;
    ViewSettings* viewSettings_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
