#include <iostream>
#include <limits>

#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtCore/QVector"
#include "QtGui/QFocusEvent"
#include "QtGui/QPainter"
#include "QtGui/QResizeEvent"

#include "GUI/LogUtils.h"

#include "App.h"
#include "AzimuthLatch.h"
#include "Configuration.h"
#include "FFTSettings.h"
#include "Settings.h"
#include "SpectrumWidget.h"
#include "ViewChanger.h"
#include "ViewEditor.h"

using namespace SideCar::GUI::Spectrum;

static int kUpdateRate = 33; // msecs between update() calls (~30 FPS)

static const char* const kShowGrid = "ShowGrid";
static const char* const kViews = "Views";

Logger::Log&
SpectrumWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.SpectrumWidget");
    return log_;
}

SpectrumWidget::SpectrumWidget(QWidget* parent) :
    Super(parent), settings_(App::GetApp()->getConfiguration()->getSettings()),
    fftSettings_(App::GetApp()->getConfiguration()->getFFTSettings()), updateTimer_(), background_(),
    azimuthLatch_(0), bins_(), points_(), viewStack_(), mouse_(), viewChanger_(0), needUpdate_(true),
    frozen_(false)
{
    static Logger::ProcLog log("Visualizer", Log());
    LOGINFO << std::endl;

    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setCursor(Qt::CrossCursor);

    connect(settings_, &Settings::powerScalingChanged, this, &SpectrumWidget::powerScalingChanged);
    connect(settings_, &Settings::sampleFrequencyChanged, this, &SpectrumWidget::recalculateX);
    connect(settings_, &Settings::colorChanged, this, &SpectrumWidget::updateColor);
    connect(settings_, &Settings::drawingModeChanged, this, &SpectrumWidget::needUpdate);
    connect(fftSettings_, &FFTSettings::fftSizeChanged, this, &SpectrumWidget::setFFTSize);

    updateColor(settings_->getQColor());

    ViewEditor* viewEditor = App::GetApp()->getViewEditor();
    connect(this, &SpectrumWidget::transformChanged, viewEditor, &ViewEditor::updateViewLimits);

    setFFTSize(fftSettings_->getFFTSize());
    updateTransform();
}

void
SpectrumWidget::setFFTSize(int fftSize)
{
    static Logger::ProcLog log("setFFTSize", Log());
    LOGINFO << fftSize << std::endl;
    bins_.resize(fftSize);
    points_.resize(fftSize);
    powerScalingChanged();
    recalculateX();
}

void
SpectrumWidget::recalculateX()
{
    double frequencyMin = fftSettings_->getFrequencyMin();
    double frequencyStep = fftSettings_->getFrequencyStep();

    for (int index = 0; index < points_.size(); ++index) {
        bins_[index].setX(frequencyMin + index * frequencyStep);
    }

    if (viewStack_.empty()) {
        viewStack_.push_back(ViewSettings(frequencyMin, -frequencyMin, -80, 0));
        updateTransform();
    }

    needUpdate();
}

void
SpectrumWidget::saveToSettings(QSettings& settings)
{
    int count = viewStack_.size();
    settings.beginWriteArray(kViews, count);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        viewStack_[index].saveToSettings(settings);
    }
    settings.endArray();
}

void
SpectrumWidget::restoreFromSettings(QSettings& settings)
{
    int count = settings.beginReadArray(kViews);
    if (count) viewStack_.clear();

    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        viewStack_.push_back(ViewSettings(settings));
    }

    settings.endArray();
    updateTransform();
}

void
SpectrumWidget::setCurrentView(const ViewSettings& view)
{
    if (view != viewStack_.back()) {
        viewStack_.back() = view;
        updateTransform();
    }
}

void
SpectrumWidget::dupView()
{
    viewStack_.push_back(viewStack_.back());
}

void
SpectrumWidget::popView()
{
    if (canPopView()) {
        ViewSettings last(viewStack_.back());
        viewStack_.pop_back();
        if (last != viewStack_.back()) { updateTransform(); }
    }
}

void
SpectrumWidget::popAllViews()
{
    ViewSettings view(viewStack_[0]);
    viewStack_.clear();
    viewStack_.push_back(view);
    updateTransform();
}

void
SpectrumWidget::setBackground(const QImage& background)
{
    static Logger::ProcLog log("setBackground", Log());
    LOGINFO << std::endl;
    background_ = QPixmap::fromImage(background);
    needUpdate();
}

void
SpectrumWidget::updateColor(const QColor& color)
{
    color_ = color;
    needUpdate();
}

void
SpectrumWidget::resizeEvent(QResizeEvent* event)
{
    static Logger::ProcLog log("resizeEvent", Log());
    LOGINFO << "width: " << width() << " height: " << height() << std::endl;
    Super::resizeEvent(event);
    updateTransform();
}

void
SpectrumWidget::updateTransform()
{
    static Logger::ProcLog log("updateTransform", Log());

    const QRectF& viewRect(getCurrentView().getBounds());
    LOGDEBUG << "left: " << viewRect.left() << " right: " << viewRect.right() << " bottom: " << viewRect.bottom()
             << " top: " << viewRect.top() << " width: " << viewRect.width() << " height: " << viewRect.height()
             << std::endl;

    double dx = viewRect.width();
    double m11 = (width() - 1.0) / dx;
    dx = -viewRect.x() * m11;

    double dy = viewRect.height();
    double m22 = -(height() - 1.0) / dy;
    dy = height() - 1.0 - viewRect.y() * m22;

    transform_.setMatrix(m11, 0.0, 0.0, m22, dx, dy);
    inverseTransform_ = transform_.inverted();

    needUpdate();

    emit transformChanged();
}

void
SpectrumWidget::powerScalingChanged()
{
    // Recalculate the constant value that we subtract from. The formula for converting from an FFTW complex
    // result (a + jb) to a db value is: magnitude = sqrt(a * a + b * b) 20 * log10((magnitude / FFTSize) /
    // pMax) where FFTSize is the number of bins in the FFT and pMax is the largest value expected from sqrt(
    // a*a + b*b). We can hoist out the divisions inside the log10() expression, leaving the following: 20 * (
    // log10(magnitude) - log10(FFTSize) - log10(pMax))
    //
    double sampleRange = settings_->getPowerMax();
    dbOffset_ = 20.0 * (::log10(bins_.size()) + ::log10(sampleRange));
}

void
SpectrumWidget::setData(const Messages::PRIMessage::Ref& msg, const fftw_complex* data)
{
    static Logger::ProcLog log("setData", Log());
    LOGINFO << "data: " << data << std::endl;

    if (azimuthLatch_) {
        if (!azimuthLatch_->check(msg->getAzimuthStart())) return;
    }

    if (frozen_) return;

    last_ = msg;

    // Caclulate powers from the FFT bin values. The FFT data comes in as complex values, partitioned into two
    // same-sized blocks, positive frequencies and negattive frequencies. The very first entry (index 0) is the
    // 0th frequency component, commonly thought of as the DC offset. We wish to display the negative
    // frequencies first, from the highest negative value down to zero, followed by the positive frequency
    // values, going out to the highest frequency. Because of this transformation, the indexing below is a bit
    // convoluted. The FFTW algorithm returns unnormalized results, meaning the returned values have a factor of
    // the FFT size in them. Removing this is usually accomplished by dividing the value by the FFT size.
    // However, since we are taking the log10 of the result, we can do away with the division by subtracting out
    // the log10 of the FFT size (plus some other stuff). See the above powerScalingChanged() method for the
    // specifics of what is made a constant.
    //
    int midPoint = bins_.size() / 2;

    // Start with the negative frequencies. We want the largest frequency component at the beginning, so reverse
    // the first half of the incoming data.
    //
    const fftw_complex* ptr = data + midPoint - 1;
    for (int index = 0; index < midPoint; ++index) { bins_[index].setY(getPowerFromMagnitude(*ptr--)); }

    // Now do the positive frequencies. Reverse the second half of the incoming data.
    //
    ptr = data + bins_.size() - 1;
    for (int index = midPoint; index < bins_.size(); ++index) {
        bins_[index].setY(getPowerFromMagnitude(*ptr--));
    }

    emit binsUpdated(bins_);

    needUpdate();
}

void
SpectrumWidget::paintEvent(QPaintEvent* event)
{
    static Logger::ProcLog log("paintEvent", Log());

    QPainter painter(this);
    if (!background_.isNull()) painter.drawPixmap(0, 0, background_);

    painter.setPen(QPen(color_));

    for (int index = 0; index < points_.size(); ++index) {
        points_[index] = transform_.map(bins_[index]);
    }

    if (settings_->getDrawingMode() == Settings::kPoints) {
        painter.drawPoints(&points_[0], points_.size());
    }
    else {
        painter.drawPolyline(&points_[0], points_.size());
    }
}

void
SpectrumWidget::showEvent(QShowEvent* event)
{
    static Logger::ProcLog log("showEvent", Log());
    LOGINFO << std::endl;

    if (!updateTimer_.isActive()) {
        updateTimer_.start(kUpdateRate, this);
        needUpdate();
    }

    Super::showEvent(event);
}

void
SpectrumWidget::hideEvent(QHideEvent* event)
{
    static Logger::ProcLog log("hideEvent", Log());
    LOGINFO << std::endl;

    if (updateTimer_.isActive()) {
        updateTimer_.stop();
    }

    Super::hideEvent(event);
}

void
SpectrumWidget::mousePressEvent(QMouseEvent* event)
{
    static Logger::ProcLog log("mousePressEvent", Log());
    LOGINFO << event->button() << ' ' << event->modifiers() << std::endl;
    Super::mousePressEvent(event);
    Q_ASSERT(viewChanger_ == 0);
    if (event->button() == Qt::LeftButton) {
        LOGDEBUG << "left button" << std::endl;
        if (event->modifiers() == 0) {
            LOGDEBUG << "zooming" << std::endl;
            viewChanger_ = new ZoomingViewChanger(this, event->pos());
            event->accept();
        } else if (event->modifiers() == Qt::ShiftModifier) {
            LOGDEBUG << "panning" << std::endl;
            viewChanger_ = new PanningViewChanger(this, event->pos());
            event->accept();
            setCursor(Qt::ClosedHandCursor);
        }
    }
}

void
SpectrumWidget::mouseMoveEvent(QMouseEvent* event)
{
    static Logger::ProcLog log("mouseReleaseEvent", Log());
    Super::mouseReleaseEvent(event);
    if (viewChanger_) {
        viewChanger_->mouseMoved(event->pos());
        needUpdate();
    }
}

void
SpectrumWidget::mouseReleaseEvent(QMouseEvent* event)
{
    static Logger::ProcLog log("mouseReleaseEvent", Log());
    Super::mouseReleaseEvent(event);
    if (viewChanger_) {
        viewChanger_->finished(event->pos());
        delete viewChanger_;
        viewChanger_ = 0;
        setCursor(event->modifiers() == Qt::ShiftModifier ? Qt::OpenHandCursor : Qt::CrossCursor);
    }
}

void
SpectrumWidget::keyPressEvent(QKeyEvent* event)
{
    if (viewChanger_) {
        if (event->key() == Qt::Key_Escape) {
            event->accept();
            delete viewChanger_;
            viewChanger_ = 0;
            setCursor(Qt::CrossCursor);
        }

        return;
    }

    if (event->key() == Qt::Key_C) {
        event->accept();
        centerAtCursor();
    } else if (event->key() == Qt::Key_Shift) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }

    Super::keyPressEvent(event);
}

void
SpectrumWidget::timerEvent(QTimerEvent* event)
{
    static Logger::ProcLog log("timerEvent", Log());

    if (event->timerId() == updateTimer_.timerId()) {
        event->accept();
        if (underMouse()) {
            QPoint newMouse = mapFromGlobal(QCursor::pos());
            if (newMouse != mouse_) {
                mouse_ = newMouse;
                emit currentCursorPosition(inverseTransform_.map(QPointF(mouse_)));
            }
        }

        if (last_) {
            emit showMessageInfo(last_);
            last_.reset();
        }

        if (needUpdate_) {
            needUpdate_ = false;
            update();
        }
    } else {
        Super::timerEvent(event);
    }
}

void
SpectrumWidget::setFrozen(bool state)
{
    frozen_ = state;
}

void
SpectrumWidget::setAzimuthLatch(AzimuthLatch* azimuthLatch)
{
    azimuthLatch_ = azimuthLatch;
}

void
SpectrumWidget::needUpdate()
{
    needUpdate_ = true;
}

void
SpectrumWidget::zoom(const QPoint& from, const QPoint& to)
{
    static Logger::ProcLog log("zoom", Log());
    LOGINFO << from.x() << ',' << from.y() << ' ' << to.x() << ',' << to.y() << std::endl;

    // Convert the mouse down point and the mouse up point to real-world values. Set up the rectangle so that
    // width()/heigth() is always positive.
    //
    QPointF f(inverseTransform_.map(QPointF(from)));
    QPointF t(inverseTransform_.map(QPointF(to)));
    QRectF fromTo(QRectF(f, QSizeF(t.x() - f.x(), t.y() - f.y())).normalized());
    LOGINFO << fromTo.left() << '-' << fromTo.right() << ' ' << fromTo.top() << '-' << fromTo.bottom()
            << std::endl;

    viewStack_.back().setBounds(QRectF(fromTo.left(), fromTo.top(), fromTo.width(), fromTo.height()));
    updateTransform();
    update();
}

void
SpectrumWidget::pan(const QPoint& from, const QPoint& to)
{
    static Logger::ProcLog log("pan", Log());
    LOGINFO << from.x() << ',' << from.y() << ' ' << to.x() << ',' << to.y() << std::endl;
    QPointF f(inverseTransform_.map(QPointF(from)));
    QPointF t(inverseTransform_.map(QPointF(to)));
    QRectF fromTo(QRectF(f, QSizeF(t.x() - f.x(), f.y() - t.y())));
    LOGINFO << fromTo.width() << ' ' << fromTo.height() << std::endl;
    QRectF bounds(viewStack_.back().getBounds());
    bounds.translate(-fromTo.width(), fromTo.height());
    viewStack_.back().setBounds(bounds);
    updateTransform();
    update();
}

void
SpectrumWidget::centerAtCursor()
{
    QPointF pos = inverseTransform_.map(QPointF(mapFromGlobal(QCursor::pos())));
    ViewSettings newView(viewStack_.back());
    newView.setBounds(newView.getBounds().translated(pos.x() - newView.getBounds().width() / 2,
                                                     pos.y() - newView.getBounds().height() / 2));
    viewStack_.push_back(newView);
    QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
    updateTransform();
}

void
SpectrumWidget::swapViews()
{
    size_t other = viewStack_.size() - 2;
    ViewSettings view(viewStack_.back());
    viewStack_.back() = viewStack_[other];
    viewStack_[other] = view;
    updateTransform();
}
