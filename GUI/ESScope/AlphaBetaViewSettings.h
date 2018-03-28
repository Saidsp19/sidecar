#ifndef SIDECAR_GUI_ESSCOPE_ALPHABETAVIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_ALPHABETAVIEWSETTINGS_H

#include "ViewSettings.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class AlphaBetaViewSettings : public ViewSettings {
    using Super = ViewSettings;

public:
    AlphaBetaViewSettings(QObject* parent, const RadarSettings* radarSettings);

private:
    bool updateViewBounds(ViewBounds& viewBounds);
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
