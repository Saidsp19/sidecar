#ifndef SIDECAR_GUI_PPIDISPLAY_MAGNIFIERVIEW_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_MAGNIFIERVIEW_H

#include "QtCore/QBasicTimer"
#include "QtOpenGL/QGLWidget"

class QSettings;

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {

class BugPlotEmitterSettings;
class PhantomCursorImaging;
class VertexColorArray;

namespace PPIDisplay {

class PPIWidget;

class MagnifierView : public QGLWidget {
    Q_OBJECT
    using Super = QGLWidget;

public:
    static Logger::Log& Log();

    /** Constructor.

        \param parent parent object
    */
    MagnifierView(PPIWidget* contents, QWidget* parent);

    void setBounds(double xCenter, double yCenter, double scale);

    void drawFrame() const;

    void save(QSettings& settings) const;

    void restore(QSettings& settings);

    void pan(double dx, double dy);

    bool getShowPhantomCursor() const { return showPhantomCursor_; }

signals:

    void currentCursorPosition(const QPointF& pos);

    void boundsChanged(double xMin, double xMax, double yMin, double yMax);

public slots:

    void zoomIn();

    void zoomOut();

    void panLeft();

    void panRight();

    void panUp();

    void panDown();

    void setShowPhantomCursor(bool state);

    void setPhantomCursor(const QPointF& pos);

    void setShowCursorPosition(bool state);

    void setCursorPosition(const QString& value);

private:
    /** Initialize the OpenGL environment.
     */
    void initializeGL();

    /** Resize the view.

        \param width width of the resized size

        \param eight of the resized size
    */
    void resizeGL(int width, int height);

    /** Draw the GL display
     */
    void paintGL();

    void showEvent(QShowEvent* event);

    void closeEvent(QCloseEvent* event);

    void leaveEvent(QEvent* event);

    void timerEvent(QTimerEvent* event);

    void mousePressEvent(QMouseEvent* event);

    void mouseMoveEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

    void wheelEvent(QWheelEvent* event);

    void keyPressEvent(QKeyEvent* event);

    void keyReleaseEvent(QKeyEvent* event);

    void updateBounds();

    void updateZoom();

    void applyZoom();

    void shift(int xd, int yd);

    PPIWidget* contents_;
    PhantomCursorImaging* phantomCursorImaging_;
    BugPlotEmitterSettings* bugPlotEmitterSettings_;
    QBasicTimer updateTimer_;
    double xCenter_;
    double yCenter_;
    double xMin_;
    double yMin_;
    double xMax_;
    double yMax_;
    double scale_;
    double zoom_;
    int zoomPower_;
    QPoint panFrom_;
    QPoint lastMouse_;
    QPointF phantomCursor_;
    QString cursorPosition_;
    bool panning_;
    bool showPhantomCursor_;
    bool showCursorPosition_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
