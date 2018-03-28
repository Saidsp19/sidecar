#ifndef SIDECAR_GUI_COLORMAPWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_COLORMAPWIDGET_H

#include "QtGui/QPalette"
#include "QtGui/QWidget"

class QImage;

namespace SideCar {
namespace GUI {

class VideoSampleCountTransform;

/** Floating tool window that shows the message time for the data shown in a PPIWidget, and the range/azimuth
    values for the cursor. The message values may be current or historical.
*/
class ColorMapWidget : public QWidget {
    Q_OBJECT
    using Super = QWidget;

public:
    /** Constructor. Creates and initializes window widgets.
     */
    ColorMapWidget(VideoSampleCountTransform* transform, QWidget* parent = 0);

signals:

    void changeColorMapType(int index);

public slots:

    void setColorMap(const QImage& colorMap);

private slots:

    void updateLabels();

    void decibelStateChange(bool newState);

    void emitChangeColorMapType();

private:
    void mouseDoubleClickEvent(QMouseEvent* event);

    class GUI;
    GUI* gui_;
    VideoSampleCountTransform* transform_;
    QPalette normalPalette_;
    QPalette dbPalette_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
