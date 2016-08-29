#ifndef SIDECAR_GUI_ESSCOPE_OFFSCREENBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_OFFSCREENBUFFER_H

#include "GUI/OffscreenBuffer.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class OffscreenBuffer : public ::SideCar::GUI::OffscreenBuffer 
{
    Q_OBJECT
    using Super = ::SideCar::GUI::OffscreenBuffer;
public:

    OffscreenBuffer(const SampleImaging* imaging, ViewSettings* viewSettings,
                    int width, int height, int textureType = -1);
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
