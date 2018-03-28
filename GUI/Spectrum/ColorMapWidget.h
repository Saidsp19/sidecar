#ifndef SIDECAR_GUI_SPECTRUM_COLORMAPWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_COLORMAPWIDGET_H

#include "QtGui/QPalette"
#include "QtGui/QWidget"

class QImage;

namespace SideCar {
namespace GUI {
namespace Spectrum {

/** Floating tool window that shows the message time for the data shown in a PPIWidget, and the range/azimuth
    values for the cursor. The message values may be current or historical.
*/
class ColorMapWidget : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    /** Constructor. Creates and initializes window widgets.
     */
    ColorMapWidget(double minCutoff, double maxCutoff, QWidget* parent = 0);

public slots:

    void setColorMap(const QImage& colorMap);

    void setMinCutoff(double minCutoff);

    void setMaxCutoff(double maxCutoff);

private slots:

    void changeColorMap();

private:
    void updateLabels();

    void mouseDoubleClickEvent(QMouseEvent* event);

    class GUI;
    GUI* gui_;
    double minCutoff_;
    double maxCutoff_;
};

} // namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
