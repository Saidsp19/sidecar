#ifndef SIDECAR_GUI_BSCOPE_OFFSCREENBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_OFFSCREENBUFFER_H

#include "GUI/OffscreenBuffer.h"

#include "ViewSettings.h"

namespace SideCar {
namespace GUI {
namespace BScope {

class ViewSettings;

class OffscreenBuffer : public ::SideCar::GUI::OffscreenBuffer
{
    Q_OBJECT
    using Super = ::SideCar::GUI::OffscreenBuffer;
public:

    OffscreenBuffer(const SampleImaging* imaging, ViewSettings* viewSettings,
                    int width, int height, int textureType = -1);

private slots:

    void viewSettingsChanged();
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
