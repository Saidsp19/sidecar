#ifdef linux
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_PROTOTYPES
#endif

#include "GUI/BugPlotEmitterSettings.h"
#include "GUI/LogUtils.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/RangeRingsImaging.h"
#include "GUI/RangeTruthsImaging.h"
#include "GUI/StencilBufferState.h"
#include "GUI/TargetPlotImaging.h"
#include "GUI/VertexGenerator.h"
#include "GUI/VideoSampleCountTransform.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "GridImaging.h"
#include "Quad2.h"
#include "RadarSettings.h"
#include "VideoImaging.h"
#include "ViewSettings.h"
#include "XYWidget.h"

using namespace Utils;
using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

Logger::Log&
XYWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.XYWidget");
    return log_;
}

QGLFormat
XYWidget::GetGLFormat()
{
    QGLFormat format = QGLFormat::defaultFormat();
    format.setAlpha(true);
    format.setAccum(false);
    format.setDepth(false);
    format.setDoubleBuffer(true);
    format.setSampleBuffers(true);
    format.setStencil(true);
    return format;
}

XYWidget::XYWidget(XYView* parent, ViewSettings* viewSettings, PlotPositionFunctor* plotPositionFunctor) :
    Super(GetGLFormat(), parent), parent_(parent), viewSettings_(viewSettings),
    plotPositionFunctor_(plotPositionFunctor), videoSampleCountTransform_(0), videoImaging_(0), extractionsImaging_(0),
    rangeTruthsImaging_(0), bugPlotsImaging_(0), bugPlotEmitterSettings_(0), gridImaging_(0), history_(0),
    xScaling_(1.0), yScaling_(1.0), xyScaling_(1.0), displayLists_(0), lastMouse_(), pendingBugPlot_(),
    lastMouseX_(0.0), lastMouseY_(0.0), hGridPositions_(), vGridPositions_(), colors_(), clearColor_(),
    slaveAlpha_(-std::numeric_limits<double>::max()), underMouse_(false)
{
    Logger::ProcLog log("XYWidget", Log());
    LOGINFO << std::endl;

    vbos_[0] = 0;
    vbos_[1] = 0;

    setCursor(Qt::CrossCursor);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setFocusPolicy(Qt::StrongFocus);

    connect(viewSettings, SIGNAL(viewChanged()), SLOT(viewSettingsChanged()));

    App* app = App::GetApp();
    Configuration* configuration = app->getConfiguration();

    radarSettings_ = configuration->getRadarSettings();
    connect(radarSettings_, SIGNAL(scansChanged(int, int, int)), SLOT(scansChanged(int, int, int)));
    connect(radarSettings_, SIGNAL(alphaMinMaxChanged(double, double)), SLOT(generateVertices()));

    history_ = configuration->getHistory();
    connect(history_, SIGNAL(alphasChanged(const AlphaIndices&)), SLOT(alphasChanged(const AlphaIndices&)));

    videoImaging_ = configuration->getVideoImaging();
    connect(videoImaging_, SIGNAL(colorChanged()), SLOT(redraw()));
    connect(videoImaging_, SIGNAL(colorMapChanged(const QImage&)), SLOT(redraw()));

    videoSampleCountTransform_ = configuration->getVideoSampleCountTransform();
    connect(videoSampleCountTransform_, SIGNAL(settingChanged()), SLOT(redraw()));

    extractionsImaging_ = configuration->getExtractionsImaging();
    rangeTruthsImaging_ = configuration->getRangeTruthsImaging();
    bugPlotsImaging_ = configuration->getBugPlotsImaging();
    gridImaging_ = configuration->getGridImaging();
    phantomCursorImaging_ = configuration->getPhantomCursorImaging();
    connect(gridImaging_, SIGNAL(settingChanged()), SLOT(makeGridLines()));
    bugPlotEmitterSettings_ = configuration->getBugPlotEmitterSettings();
}

XYWidget::~XYWidget()
{
    Logger::ProcLog log("~XYWidget", Log());
    makeCurrent();
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    if (vbos_[0]) GLEC(glDeleteBuffers(kNumVBOs, vbos_));
    if (displayLists_) GLEC(glDeleteLists(displayLists_, kNumLists));
}

void
XYWidget::initializeGL()
{
    Logger::ProcLog log("initializeGL", Log());
    LOGINFO << std::endl;

    xScans_ = getXScans();
    yScans_ = getYScans();
    gridSize_ = xScans_ * yScans_;

    displayLists_ = glGenLists(kNumLists);
    if (displayLists_ == 0) {
        Utils::Exception ex("failed to allocate OpenGL display lists");
        log.thrower(ex);
    }

    GLEC(glDisable(GL_COLOR_SUM));
    GLEC(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GLEC(glEnable(GL_MULTISAMPLE));
    GLEC(glEnable(GL_POINT_SMOOTH));
    GLEC(glHint(GL_POINT_SMOOTH_HINT, GL_NICEST));
    GLEC(glEnable(GL_LINE_SMOOTH));
    GLEC(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));
    GLEC(glClearStencil(1));
    GLEC(glStencilFunc(GL_EQUAL, 1, 1));
    GLEC(glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO));

    GLEC(glMatrixMode(GL_MODELVIEW));
    GLEC(glLoadIdentity());
    GLEC(glMatrixMode(GL_PROJECTION));
    GLEC(glLoadIdentity());

    setViewTransform();

    GLEC(glEnableClientState(GL_VERTEX_ARRAY));
    GLEC(glEnableClientState(GL_COLOR_ARRAY));

    makeBuffers();
}

void
XYWidget::paintGL()
{
    static Logger::ProcLog log("paintGL", Log());

    GLEC(glDisable(GL_BLEND));
    GLEC(glClearColor(clearColor_.red, clearColor_.green, clearColor_.blue, 1.0f));
    GLEC(glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

    GLEC(glLoadIdentity());
    GLEC(glOrtho(viewSettings_->getXMin(), viewSettings_->getXMax(), viewSettings_->getYMin(), viewSettings_->getYMax(),
                 -1.0, 1.0));
    GLEC(glDrawArrays(GL_QUADS, 0, gridSize_ * Quad2::kVerticesPerQuad));
    GLEC(glEnable(GL_BLEND));

    StencilBufferState stencilState;
    if (gridImaging_->isEnabled()) {
        stencilState.use();
        GLEC(glCallList(getDisplayList(kGridLines)));
    }

    if (rangeTruthsImaging_->isEnabled()) {
        const TargetPlotListList& rangeTruths(history_->getRangeTruths());
        if (!rangeTruths.empty()) {
            stencilState.use();
            rangeTruthsImaging_->setPlotPositionFunctor(plotPositionFunctor_);
            rangeTruthsImaging_->render(this, rangeTruths);
        }
    }

    if (extractionsImaging_->isEnabled()) {
        const TargetPlotList& extractions(history_->getExtractions());
        if (!extractions.empty()) {
            stencilState.use();
            extractionsImaging_->setPlotPositionFunctor(plotPositionFunctor_);
            extractionsImaging_->render(this, extractions);
        }
    }

    if (bugPlotsImaging_->isEnabled()) {
        const TargetPlotList& bugPlots(history_->getBugPlots());
        if (!bugPlots.empty()) {
            stencilState.use();
            bugPlotsImaging_->setPlotPositionFunctor(plotPositionFunctor_);
            bugPlotsImaging_->render(this, bugPlots);
        }
    }

    if (!pendingBugPlot_.isNull()) {
        GLdouble x1, y1, x2, y2;
        localToRealWorld(pendingBugPlot_.x(), pendingBugPlot_.y(), x1, y1);
        localToRealWorld(pendingBugPlot_.x() + 10, pendingBugPlot_.y() + 10, x2, y2);
        GLdouble dx = x2 - x1;
        GLdouble dy = y2 - y1;
        GLEC(glLineWidth(2.0));
        GLEC(glColor3f(0.0, 1.0, 1.0));
        glBegin(GL_LINE_LOOP);
        glVertex2f(x2, y2);
        glVertex2f(x1 - dx, y2);
        glVertex2f(x1 - dx, y1 - dy);
        glVertex2f(x2, y1 - dy);
        glEnd();
    }

    GLEC(glLineWidth(1.0));
    GLEC(glColor4f(0.5, 1.0, 0.5, 0.5));

    if (underMouse_) {
        if (rubberBanding_) {
            glColor4f(1.0, 1.0, 1.0, 0.5);
            glLineWidth(1.0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(magStartX_, magStartY_);
            glVertex2f(magEndX_, magStartY_);
            glVertex2f(magEndX_, magEndY_);
            glVertex2f(magStartX_, magEndY_);
            glEnd();
        }

        QPoint newMouse = mapFromGlobal(QCursor::pos());
        if (newMouse != lastMouse_) {
            lastMouse_ = newMouse;
            localToRealWorld(newMouse.x(), newMouse.y(), lastMouseX_, lastMouseY_);
            emit cursorMoved(lastMouseX_, lastMouseY_);
        }

        glBegin(GL_LINES);
        glVertex2f(viewSettings_->getXMin(), lastMouseY_);
        glVertex2f(viewSettings_->getXMax(), lastMouseY_);
        glVertex2f(lastMouseX_, viewSettings_->getYMin());
        glVertex2f(lastMouseX_, viewSettings_->getYMax());
        glEnd();
    } else {
        glBegin(GL_LINES);
        glVertex2f(slaveAlpha_, viewSettings_->getYMin());
        glVertex2f(slaveAlpha_, viewSettings_->getYMax());
        glEnd();
    }
}

void
XYWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() == 0) {
            event->accept();
            magStartPos_ = event->pos();
            localToRealWorld(event->x(), event->y(), magStartX_, magStartY_);
            magEndX_ = magStartX_;
            magEndY_ = magStartY_;
            rubberBanding_ = true;
        } else if (event->modifiers() == Qt::ControlModifier) {
            event->accept();
            if (pendingBugPlot_ == event->pos()) {
                clearPendingBugPlot();
            } else {
                pendingBugPlot_ = event->pos();
                emit bugged();
            }
        }
    }

    Super::mousePressEvent(event);
}

void
XYWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (rubberBanding_) {
        localToRealWorld(event->x(), event->y(), magEndX_, magEndY_);
    } else if (!pendingBugPlot_.isNull() && event->modifiers() == Qt::ControlModifier) {
        event->accept();
        pendingBugPlot_ = event->pos();
        emit bugged();
    }
}

void
XYWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (rubberBanding_) {
            event->accept();
            rubberBanding_ = false;
            if (std::abs(magStartPos_.x() - event->x()) > 10 && std::abs(magStartPos_.y() - event->y()) > 10) {
                localToRealWorld(event->x(), event->y(), magEndX_, magEndY_);
                if (magStartX_ > magEndX_) std::swap(magStartX_, magEndX_);
                if (magStartY_ > magEndY_) std::swap(magStartY_, magEndY_);
                viewSettings_->push(magStartX_, magEndX_, magStartY_, magEndY_);
            }
        }
    }

    Super::mouseReleaseEvent(event);
}

void
XYWidget::keyPressEvent(QKeyEvent* event)
{
    if (rubberBanding_) {
        if (event->key() == Qt::Key_Escape) {
            event->accept();
            rubberBanding_ = false;
        }
        return;
    }

    if (event->key() == Qt::Key_C) {
        event->accept();
        centerAtCursor();
    }

    Super::keyPressEvent(event);
}

void
XYWidget::wheelEvent(QWheelEvent* event)
{
    event->accept();
}

void
XYWidget::enterEvent(QEvent* event)
{
    underMouse_ = true;
    setFocus(Qt::MouseFocusReason);
    Super::enterEvent(event);
}

void
XYWidget::leaveEvent(QEvent* event)
{
    if (rubberBanding_) rubberBanding_ = false;
    underMouse_ = false;
    Super::leaveEvent(event);
}

void
XYWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    setViewTransform();
}

void
XYWidget::setViewTransform()
{
    xScaling_ = viewSettings_->getXSpan() / (width() - 1);
    yScaling_ = viewSettings_->getYSpan() / (height() - 1);
    xyScaling_ = xScaling_ / yScaling_;
}

void
XYWidget::viewSettingsChanged()
{
    makeCurrent();
    setViewTransform();
    makeGridLines();
}

void
XYWidget::localToRealWorld(int xIn, int yIn, GLdouble& xOut, GLdouble& yOut) const
{
    xOut = xIn * xScaling_ + viewSettings_->getXMin();
    yOut = viewSettings_->getYMax() - yIn * yScaling_;
}

void
XYWidget::setGridPositions(const std::vector<float>& hGridPositions, const std::vector<float>& vGridPositions)
{
    hGridPositions_ = hGridPositions;
    vGridPositions_ = vGridPositions;
    makeGridLines();
}

void
XYWidget::makeGridLines()
{
    Logger::ProcLog log("makeGridLines", Log());
    LOGINFO << "displayLists: " << displayLists_ << std::endl;

    if (displayLists_ == 0) return;

    makeCurrent();

    GLEC(glNewList(getDisplayList(kGridLines), GL_COMPILE));
    {
        gridImaging_->getColor().use();
        double lineWidth = gridImaging_->getSize();
        GLEC(glLineWidth(lineWidth));

        double xMin = viewSettings_->getXMin();
        double xMax = viewSettings_->getXMax();
        double xSpan = xMax - xMin;
        double yMin = viewSettings_->getYMin();
        double yMax = viewSettings_->getYMax();
        double ySpan = yMax - yMin;

        glBegin(GL_LINES);

        // Generate points for the vertical grid lines, from xMin to xMax
        //
        for (size_t index = 0; index < hGridPositions_.size(); ++index) {
            float x = hGridPositions_[index] * xSpan + xMin;
            glVertex2f(x, yMin);
            glVertex2f(x, yMax);
        }

        // Generate points for the horizontal grid lines, from yMin to yMax
        //
        for (size_t index = 0; index < vGridPositions_.size(); ++index) {
            float y = vGridPositions_[index] * ySpan + yMin;
            glVertex2f(xMin, y);
            glVertex2f(xMax, y);
        }

        glEnd();
    }
    GLEC(glEndList());
}

void
XYWidget::setSlaveAlpha(double value)
{
    slaveAlpha_ = value;
    parent_->setSlaveAlpha(value);
}

QPointF
XYWidget::getPendingBugPlot() const
{
    if (pendingBugPlot_.isNull()) return QPointF();
    GLdouble x, y;
    localToRealWorld(pendingBugPlot_.x(), pendingBugPlot_.y(), x, y);
    return QPointF(x, y);
}

void
XYWidget::centerAtCursor()
{
    QPoint pos(mapFromGlobal(QCursor::pos()));
    double x, y;
    localToRealWorld(pos.x(), pos.y(), x, y);
    double xSpan2 = viewSettings_->getXSpan() / 2.0;
    double ySpan2 = viewSettings_->getYSpan() / 2.0;
    viewSettings_->push(x - xSpan2, x + xSpan2, y - ySpan2, y + ySpan2);
    pos = QPoint(width() / 2, height() / 2);
    QCursor::setPos(mapToGlobal(pos));
}

void
XYWidget::changeZoom(int factor)
{
}

void
XYWidget::scansChanged(int alphaScans, int betaScans, int rangeScans)
{
    Logger::ProcLog log("scansChanged", Log());
    LOGERROR << alphaScans << ' ' << betaScans << std::endl;
    if (xScans_ != getXScans() || yScans_ != getYScans()) {
        xScans_ = getXScans();
        yScans_ = getYScans();
        gridSize_ = xScans_ * yScans_;
        if (displayLists_) makeBuffers();
    }
}

void
XYWidget::makeBuffers()
{
    Logger::ProcLog log("makeBuffers", Log());
    LOGERROR << "gridSize: " << gridSize_ << std::endl;

    makeCurrent();

    if (vbos_[0]) {
        GLEC(glDeleteBuffers(kNumVBOs, vbos_));
        vbos_[0] = 0;
    }

    glGenBuffers(kNumVBOs, vbos_);

    clearColor_ = videoImaging_->getColor(videoSampleCountTransform_->transform(DataContainer::GetMinValue()));
    colors_.clear();
    colors_.resize(gridSize_, QuadColor(clearColor_));

    // Initialize the color buffer and setup our context to use the color VBO.
    //
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, vbos_[kColors]));
    GLEC(glColorPointer(Color3::kValuesPerColor, GL_FLOAT, 0, 0));
    GLEC(glBufferData(GL_ARRAY_BUFFER, colors_.byteSize(), colors_, GL_DYNAMIC_DRAW));
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, 0));

    generateVertices();
}

void
XYWidget::generateVertices()
{
    Logger::ProcLog log("generateVertices", Log());

    Quad2Vector vertices;
    vertices.reserve(gridSize_);

    GLfloat dx = (getXMaxMax() - getXMinMin()) / xScans_;
    GLfloat dy = (getYMaxMax() - getYMinMin()) / yScans_;

    for (int x = 0; x < getXScans(); ++x) {
        GLfloat x1 = x * dx + getXMinMin();
        GLfloat x2 = x1 + dx;
        for (int y = 0; y < getYScans(); ++y) {
            GLfloat y1 = y * dy + getYMinMin();
            GLfloat y2 = y1 + dy;
            vertices.push_back(Quad2(x1, y1, x2, y2));
        }
    }

    // Initialize the vertex buffer and setup our context to use the vertex.
    //
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, vbos_[kVertices]));
    GLEC(glVertexPointer(Quad2::kValuesPerVertex, GL_FLOAT, 0, 0));
    GLEC(glBufferData(GL_ARRAY_BUFFER, vertices.byteSize(), vertices, GL_STATIC_DRAW));
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void
XYWidget::clear()
{
    static Logger::ProcLog log("clear", Log());
    clearColor_ = videoImaging_->getColor(videoSampleCountTransform_->transform(DataContainer::GetMinValue()));
    colors_.clear();
    colors_.resize(gridSize_, QuadColor(clearColor_));
    makeCurrent();
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, vbos_[kColors]));
    GLEC(glBufferSubData(GL_ARRAY_BUFFER, 0, colors_.byteSize(), colors_));
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void
XYWidget::redraw()
{
    static Logger::ProcLog log("redraw", Log());

    if (!vbos_[0]) return;

    makeCurrent();

    clearColor_ = videoImaging_->getColor(videoSampleCountTransform_->transform(DataContainer::GetMinValue()));

    colors_.clear();
    fillColors();

    GLEC(glBindBuffer(GL_ARRAY_BUFFER, vbos_[kColors]));
    GLEC(glBufferSubData(GL_ARRAY_BUFFER, 0, colors_.byteSize(), colors_));
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void
XYWidget::updateColors(int vertexOffset)
{
    static Logger::ProcLog log("updateColors", Log());
    makeCurrent();
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, vbos_[kColors]));
    int offset = colors_.GetByteOffset(vertexOffset);
    int byteSize = colors_.byteSize();
    GLEC(glBufferSubData(GL_ARRAY_BUFFER, offset, byteSize, colors_));
    GLEC(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
