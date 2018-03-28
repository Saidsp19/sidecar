#ifndef SIDECAR_GUI_PPIDISPLAY_OFFSCREENBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_OFFSCREENBUFFER_H

#include "GUI/OffscreenBuffer.h"
#include "Utils/SineCosineLUT.h"

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class OffscreenBuffer : public ::SideCar::GUI::OffscreenBuffer {
    Q_OBJECT
    using Super = ::SideCar::GUI::OffscreenBuffer;

public:
    OffscreenBuffer(const SampleImaging* imaging, double rangeMax, int width, int height, int textureType = -1);

protected slots:

    /** Notification handler invoked when the maximum range view setting changes.

        \param value new maximum range value
    */
    virtual void rangeMaxChanged(double value);
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
