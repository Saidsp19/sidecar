#ifndef SIDECAR_GUI_SPECTRUM_POWERSCALEWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_POWERSCALEWIDGET_H

#include "ScaleWidget.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class PowerScaleWidget : public ScaleWidget {
public:
    PowerScaleWidget(QWidget* parent = 0, Qt::Orientation orientation = Qt::Vertical) : ScaleWidget(parent, orientation)
    {
    }

    QString formatTickTag(double value);
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
