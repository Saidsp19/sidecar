#include <cmath>

#include "QtCore/QSettings"
#include "QtCore/QTimerEvent"

#include "GUI/BugPlotEmitterSettings.h"
#include "GUI/LogUtils.h"
#include "GUI/PhantomCursorImaging.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "CursorPosition.h"
#include "History.h"
#include "MagnifierView.h"
#include "PPIWidget.h"

using namespace SideCar::GUI::PPIDisplay;

static int kUpdateRate = 33; // msecs between update() calls (~30 FPS)

Logger::Log&
MagnifierView::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.MagnifierView");
    return log_;
}

MagnifierView::MagnifierView(PPIWidget* contents, QWidget* parent) :
    QGLWidget(parent, contents), contents_(contents), updateTimer_(), zoomPower_(0), cursorPosition_(), panning_(false),
    showPhantomCursor_(true), showCursorPosition_(true)
{
    App* app = App::GetApp();
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setFocusPolicy(Qt::ClickFocus);

    phantomCursorImaging_ = app->getConfiguration()->getPhantomCursorImaging();
    bugPlotEmitterSettings_ = app->getConfiguration()->getBugPlotEmitterSettings();

    connect(app, SIGNAL(phantomCursorChanged(const QPointF&)), SLOT(setPhantomCursor(const QPointF&)));
    connect(this, SIGNAL(currentCursorPosition(const QPointF&)), app, SLOT(setPhantomCursor(const QPointF&)));
}

void
MagnifierView::setPhantomCursor(const QPointF& pos)
{
    phantomCursor_ = pos;
}

void
MagnifierView::setBounds(double xCenter, double yCenter, double scale)
{
    Logger::ProcLog log("setBounds", Log());
    LOGINFO << "xCenter: " << xCenter << " yCenter: " << yCenter << " scale: " << scale << std::endl;
    xCenter_ = xCenter;
    yCenter_ = yCenter;
    scale_ = scale;
    updateZoom();
    updateBounds();
}

void
MagnifierView::updateBounds()
{
    double w2 = width() * zoom_ / 2.0;
    double h2 = height() * zoom_ / 2.0;
    xMin_ = xCenter_ - w2;
    xMax_ = xCenter_ + w2;
    yMin_ = yCenter_ - h2;
    yMax_ = yCenter_ + h2;
    emit boundsChanged(xMin_, xMax_, yMin_, yMax_);
}

void
MagnifierView::initializeGL()
{
    contents_->initializeContext(0);
}

void
MagnifierView::resizeGL(int width, int height)
{
    Logger::ProcLog log("resizeGL", Log());
    LOGINFO << isVisible() << std::endl;
    glViewport(0, 0, width, height);
    updateBounds();
}

void
MagnifierView::updateZoom()
{
    zoom_ = scale_ * ::pow(0.8, zoomPower_);
}

void
MagnifierView::paintGL()
{
    glLoadIdentity();
    glOrtho(xMin_, xMax_, yMin_, yMax_, -1.0, 1.0);
    contents_->renderScene(this);
    if (!underMouse() && !panning_ && showPhantomCursor_) {
        phantomCursorImaging_->drawCursor(phantomCursor_, zoom_, zoom_);
    }
}

void
MagnifierView::showEvent(QShowEvent* event)
{
    if (!updateTimer_.isActive()) { updateTimer_.start(kUpdateRate, this); }
    Super::showEvent(event);
}

void
MagnifierView::closeEvent(QCloseEvent* event)
{
    if (updateTimer_.isActive()) { updateTimer_.stop(); }
    Super::closeEvent(event);
}

void
MagnifierView::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == updateTimer_.timerId()) {
        update();
        if (underMouse()) {
            phantomCursor_ = phantomCursorImaging_->InvalidCursor();
            QPoint newMouse = mapFromGlobal(QCursor::pos());
            if (newMouse != lastMouse_) {
                lastMouse_ = newMouse;
                double rx = xMin_ + lastMouse_.x() * zoom_;
                double ry = yMax_ - lastMouse_.y() * zoom_;
                CursorPosition pos(rx, ry);
                contents_->updateCursorPosition(pos);
                setCursorPosition(pos.getToolTip());
                emit currentCursorPosition(pos.getXY());
            }
        }
    }
    Super::timerEvent(event);
}

void
MagnifierView::drawFrame() const
{
    glBegin(GL_LINE_LOOP);
    glVertex2f(xMin_, yMin_);
    glVertex2f(xMax_, yMin_);
    glVertex2f(xMax_, yMax_);
    glVertex2f(xMin_, yMax_);
    glEnd();
}

void
MagnifierView::save(QSettings& settings) const
{
    settings.setValue("xCenter", xCenter_);
    settings.setValue("yCenter", yCenter_);
    settings.setValue("scale", scale_);
    settings.setValue("zoomPower", zoomPower_);
    settings.setValue("showPhantomCursor", showPhantomCursor_);
    settings.setValue("showCursorPosition", showCursorPosition_);
}

void
MagnifierView::restore(QSettings& settings)
{
    xCenter_ = settings.value("xCenter").toDouble();
    yCenter_ = settings.value("yCenter").toDouble();
    scale_ = settings.value("scale").toDouble();
    zoomPower_ = settings.value("zoomPower").toInt();
    showPhantomCursor_ = settings.value("showPhantomCursor", true).toBool();
    showCursorPosition_ = settings.value("showCursorPosition", true).toBool();
    updateZoom();
    updateBounds();
}

void
MagnifierView::zoomIn()
{
    ++zoomPower_;
    applyZoom();
}

void
MagnifierView::zoomOut()
{
    --zoomPower_;
    applyZoom();
}

void
MagnifierView::panLeft()
{
    shift(-1, 0);
}

void
MagnifierView::panRight()
{
    shift(1, 0);
}

void
MagnifierView::panDown()
{
    shift(0, -1);
}

void
MagnifierView::panUp()
{
    shift(0, 1);
}

void
MagnifierView::shift(int xd, int yd)
{
    double dx = (xMax_ - xMin_) / 4.0 * xd;
    double dy = (yMax_ - yMin_) / 4.0 * yd;
    pan(dx, dy);
}

void
MagnifierView::pan(double dx, double dy)
{
    xCenter_ += dx;
    yCenter_ += dy;
    updateBounds();
}

void
MagnifierView::applyZoom()
{
    updateZoom();
    updateBounds();
}

void
MagnifierView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() == Qt::ShiftModifier) {
            event->accept();
            setCursor(Qt::ClosedHandCursor);
            panFrom_ = event->pos();
            panning_ = true;
        } else if (event->modifiers() == Qt::ControlModifier) {
            event->accept();
            double rx = xMin_ + event->x() * zoom_;
            double ry = yMax_ - event->y() * zoom_;
            double azimuth = Utils::normalizeRadians(::atan2(rx, ry));
            double range = ::sqrt(rx * rx + ry * ry);
            Messages::BugPlot::Ref msg = bugPlotEmitterSettings_->addBugPlot(range, azimuth, 0.0);
            if (msg) { App::GetApp()->getHistory()->addBugPlot(msg); }
        }
    }

    Super::mousePressEvent(event);
}

void
MagnifierView::mouseMoveEvent(QMouseEvent* event)
{
    if (event->modifiers() == Qt::ShiftModifier && !panning_) { setCursor(Qt::OpenHandCursor); }

    if (panning_) {
        setCursor(Qt::ClosedHandCursor);
        QPoint panTo(event->pos());
        double dx = (panTo.x() - panFrom_.x()) * zoom_;
        double dy = (panTo.y() - panFrom_.y()) * zoom_;
        panFrom_ = panTo;
        pan(-dx, dy);
    }

    Super::mouseMoveEvent(event);
}

void
MagnifierView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::CrossCursor);
        event->accept();
        if (panning_) {
            panning_ = false;
            if (event->modifiers() == Qt::ShiftModifier) setCursor(Qt::OpenHandCursor);
        }
    }
    Super::mouseReleaseEvent(event);
}

void
MagnifierView::wheelEvent(QWheelEvent* event)
{
    static Logger::ProcLog log("wheelEvent", Log());
    LOGINFO << event->delta() << std::endl;
    event->accept();

    // The delta() value is given in 1/8ths of a degree. Apparently the
    // standard for the scroll wheel is to report in 15 degree steps. Thus the
    // division by 120.
    //
    int steps = event->delta() / 120;
    zoomPower_ += steps;
    applyZoom();
    Super::wheelEvent(event);
}

void
MagnifierView::keyPressEvent(QKeyEvent* event)
{
    if (QApplication::mouseButtons() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
    } else if (event->key() == Qt::Key_Shift) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
    Super::keyPressEvent(event);
}

void
MagnifierView::keyReleaseEvent(QKeyEvent* event)
{
    if (QApplication::mouseButtons() == Qt::LeftButton)
        setCursor(Qt::ClosedHandCursor);
    else
        setCursor(Qt::CrossCursor);
    Super::keyPressEvent(event);
}

void
MagnifierView::setShowPhantomCursor(bool state)
{
    showPhantomCursor_ = state;
    update();
}

void
MagnifierView::leaveEvent(QEvent* event)
{
    phantomCursor_ = PhantomCursorImaging::InvalidCursor();
    Super::leaveEvent(event);
}

void
MagnifierView::setShowCursorPosition(bool state)
{
    showCursorPosition_ = state;
    setToolTip("");
    if (state) { setToolTip(cursorPosition_); }
}

void
MagnifierView::setCursorPosition(const QString& value)
{
    if (value != cursorPosition_) {
        cursorPosition_ = value;
        if (showCursorPosition_) { setToolTip(cursorPosition_); }
    } else {
        setToolTip("");
        setToolTip(value);
    }
}
