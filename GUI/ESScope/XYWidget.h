#ifndef SIDECAR_GUI_BSCOPE_XYWIDGET_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_XYWIDGET_H

#include <vector>

#include "QtCore/QPointF"
#include "QtOpenGL/QGLWidget"

#include "GUI/Utils.h"

#include "Color3.h"
#include "History.h"
#include "XYView.h"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {

class BugPlotEmitterSettings;
class PhantomCursorImaging;
class PlotPositionFunctor;
class TargetPlotImaging;
class VertexGenerator;
class VideoSampleCountTransform;

namespace ESScope {

class GridImaging;
class History;
class RadarSettings;
class VideoImaging;
class ViewSettings;

class XYWidget : public QGLWidget {
    Q_OBJECT
    using Super = QGLWidget;

public:
    static Logger::Log& Log();

    /** Obtain the OpenGL configuration we want to use.

        \return OpenGL format
    */
    static QGLFormat GetGLFormat();

    /** Constructor.

        \param parent parent object
    */
    XYWidget(XYView* parent, ViewSettings* viewSettings, PlotPositionFunctor* plotPositionFunctor);

    ~XYWidget();

    void localToRealWorld(int x, int y, GLdouble& alpha, GLdouble& beta) const;

    void setGridPositions(const std::vector<float>& alpha, const std::vector<float>& beta);

    double getXScaling() const { return xScaling_; }
    double getYScaling() const { return yScaling_; }
    double getXYScaling() const { return xyScaling_; }

    virtual int getXScans() const = 0;
    virtual int getYScans() const = 0;
    virtual double getXMinMin() const = 0;
    virtual double getXMaxMax() const = 0;
    virtual double getYMinMin() const = 0;
    virtual double getYMaxMax() const = 0;

    QPointF getPendingBugPlot() const;

    void clearPendingBugPlot() { pendingBugPlot_ = QPoint(); }

signals:

    void cursorMoved(double x, double y);

    void bugged();

public slots:

    void setSlaveAlpha(double value);

    void redraw();

    void clear();

protected slots:

    virtual void alphasChanged(const AlphaIndices& indices) = 0;

    void generateVertices();

private slots:

    void makeGridLines();

    void scansChanged(int alphaScans, int betaScans, int rangeScans);

    void viewSettingsChanged();

protected:
    enum ListIndex { kGridLines = 0, kNumLists };

    enum VBOIndex { kVertices = 0, kColors, kNumVBOs };

    GLuint getDisplayList(int index) const { return displayLists_ + index; }

    void initializeGL();

    void makeBuffers();

    void paintGL();

    void updateColors(int offset);

    void resizeGL(int width, int height);

    void setViewTransform();

    void mousePressEvent(QMouseEvent* event);

    void mouseMoveEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

    void keyPressEvent(QKeyEvent* event);

    void wheelEvent(QWheelEvent* event);

    void enterEvent(QEvent* event);

    void leaveEvent(QEvent* event);

    virtual void fillColors() = 0;

    void centerAtCursor();

    void changeZoom(int factor);

    GLuint vbos_[kNumVBOs];
    XYView* parent_;
    ViewSettings* viewSettings_;
    PlotPositionFunctor* plotPositionFunctor_;
    VideoSampleCountTransform* videoSampleCountTransform_;
    VideoImaging* videoImaging_;
    RadarSettings* radarSettings_;
    TargetPlotImaging* extractionsImaging_;
    TargetPlotImaging* rangeTruthsImaging_;
    TargetPlotImaging* bugPlotsImaging_;
    BugPlotEmitterSettings* bugPlotEmitterSettings_;
    GridImaging* gridImaging_;
    PhantomCursorImaging* phantomCursorImaging_;
    History* history_;
    int xScans_, yScans_, gridSize_;
    double xScaling_;
    double yScaling_;
    double xyScaling_;
    GLuint displayLists_;
    QPoint lastMouse_;
    QPoint pendingBugPlot_;
    GLdouble lastMouseX_;
    GLdouble lastMouseY_;
    std::vector<float> hGridPositions_;
    std::vector<float> vGridPositions_;
    QuadColorVector colors_;
    Color clearColor_;
    QPoint magStartPos_;
    double magStartX_;
    double magStartY_;
    double magEndX_;
    double magEndY_;
    double slaveAlpha_;
    bool rubberBanding_;
    bool underMouse_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
