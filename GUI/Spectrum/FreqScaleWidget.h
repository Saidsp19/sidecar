#ifndef SIDECAR_GUI_SPECTRUM_FREQSCALEWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_FREQSCALEWIDGET_H

#include "ScaleWidget.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Spectrum {

class FreqScaleWidget : public ScaleWidget {
public:
    FreqScaleWidget(QWidget* parent = 0, Qt::Orientation orientation = Qt::Horizontal) :
        ScaleWidget(parent, orientation)
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
