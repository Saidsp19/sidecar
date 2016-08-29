#ifndef SIDECAR_GUI_ESSCOPE_ALPHARANGEVIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_ALPHARANGEVIEWSETTINGS_H

#include "ViewSettings.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class AlphaRangeViewSettings : public ViewSettings
{
    using Super = ViewSettings;
public:

    AlphaRangeViewSettings(QObject* parent,
                           const RadarSettings* radarSettings);

private:

    bool updateViewBounds(ViewBounds& viewBounds);
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
