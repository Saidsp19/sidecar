#include "QtCore/QSettings"
#include "QtGui/QFocusEvent"
#include "QtGui/QPainter"
#include "QtGui/QResizeEvent"

#include "GUI/LogUtils.h"

#include "App.h"
#include "AzimuthLatch.h"
#include "ChannelConnection.h"
#include "ChannelConnectionWindow.h"
#include "DisplayView.h"
#include "HistoryPosition.h"
#include "PeakBarCollection.h"
#include "VideoChannel.h"
#include "ViewChanger.h"
#include "ViewEditor.h"
#include "Visualizer.h"

static int kUpdateRate = 67; // msecs between update() calls (~15 FPS)

static const char* const kShowGrid = "ShowGrid";
static const char* const kChannelConnections = "ChannelConnections";
static const char* const kChannelName = "ChannelName";
static const char* const kVisible = "Visible";
static const char* const kShowPeakBars = "ShowPeakBars";
static const char* const kViews = "Views";

using namespace SideCar::GUI::AScope;

Logger::Log&
Visualizer::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.Visualizer");
    return log_;
}

Visualizer::Visualizer(DisplayView* parent, AzimuthLatch* azimuthLatch) :
    Super(parent), azimuthLatch_(azimuthLatch), historyPosition_(new HistoryPosition(this, azimuthLatch)),
    updateTimer_(), background_(), connections_(), connectionNames_(), viewStack_(), transform_(), inverseTransform_(),
    viewChanger_(0), needUpdate_(false), frozen_(false), showGrid_(true), showPeakBars_(true)
{
    static Logger::ProcLog log("Visualizer", Log());
    LOGINFO << std::endl;

    setBackgroundRole(QPalette::Shadow);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::ClickFocus);

    ViewEditor* viewEditor = App::GetApp()->getViewEditor();
    connect(this, SIGNAL(transformChanged()), viewEditor, SLOT(updateViewLimits()));

    connect(historyPosition_, SIGNAL(viewChanged()), SLOT(needUpdate()));

    viewStack_.clear();
    viewStack_.push_back(viewEditor->makeView());
    updateTransform();
}

Visualizer::~Visualizer()
{
    static Logger::ProcLog log("~Visualizer", Log());
    LOGINFO << std::endl;
    for (int index = 0; index < connections_.count(); ++index) {
        LOGDEBUG << connections_[index] << std::endl;
        delete connections_[index];
    }
}

bool
Visualizer::isFrozen() const
{
    return historyPosition_->isViewingPast();
}

void
Visualizer::setFrozen(bool state)
{
    historyPosition_->viewingPast(state);
}

void
Visualizer::duplicate(const Visualizer* other)
{
    static Logger::ProcLog log("duplicate", Log());
    LOGINFO << std::endl;

    showGrid_ = other->showGrid_;
    showPeakBars_ = other->showPeakBars_;

    for (int index = 0; index < other->connections_.size(); ++index) {
        ChannelConnection* connection = other->connections_[index];
        addVideoChannel(connection->getChannel(), connection->isVisible(), connection->isShowingPeakBars());
    }

    viewStack_ = other->viewStack_;
    updateTransform();
}

void
Visualizer::saveToSettings(QSettings& settings)
{
    Logger::ProcLog log("saveToSettings", Log());
    LOGINFO << settings.group() << std::endl;

    historyPosition_->saveToSettings(settings);

    settings.setValue(kShowGrid, showGrid_);
    settings.setValue(kShowPeakBars, showPeakBars_);

    int count = connections_.size();
    settings.beginWriteArray(kChannelConnections, count);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        ChannelConnection* connection = connections_[index];
        settings.setValue(kChannelName, connection->getChannel().getName());
        settings.setValue(kVisible, connection->isVisible());
        settings.setValue(kShowPeakBars, connection->isShowingPeakBars());
    }
    settings.endArray();

    count = viewStack_.size();
    settings.beginWriteArray(kViews, count);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        viewStack_[index].saveToSettings(settings);
    }
    settings.endArray();

    LOGINFO << "done" << std::endl;
}

void
Visualizer::restoreFromSettings(QSettings& settings)
{
    Logger::ProcLog log("restoreFromSettings", Log());
    LOGINFO << settings.group() << std::endl;

    historyPosition_->restoreFromSettings(settings);

    showGrid_ = settings.value(kShowGrid, true).toBool();
    showPeakBars_ = settings.value(kShowPeakBars, true).toBool();

    if (connections_.size()) {
        connectionNames_.clear();
        for (int index = 0; index < connections_.size(); ++index) {
            VideoChannel* channel = removeChannelConnection(index);
            if (!channel->isDisplayed()) delete channel;
        }
        connections_.clear();
    }

    ChannelConnectionWindow* channelConnectionWindow = App::GetApp()->getChannelConnectionWindow();
    int count = settings.beginReadArray(kChannelConnections);
    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        QString channelName = settings.value(kChannelName).toString();
        VideoChannel* channel = channelConnectionWindow->getVideoChannel(channelName);
        bool showPeakBars = settings.value(kShowPeakBars, false).toBool();
        bool visible = settings.value(kVisible, true).toBool();
        addVideoChannel(*channel, visible, showPeakBars);
    }
    settings.endArray();

    count = settings.beginReadArray(kViews);
    if (count) viewStack_.clear();

    for (int index = 0; index < count; ++index) {
        settings.setArrayIndex(index);
        viewStack_.push_back(ViewSettings(settings));
    }
    settings.endArray();

    updateTransform();

    LOGINFO << "done" << std::endl;
}

void
Visualizer::setBackground(const QImage& background)
{
    static Logger::ProcLog log("setBackground", Log());
    LOGINFO << std::endl;
    background_ = QPixmap::fromImage(background);
    needUpdate();
}

void
Visualizer::addVideoChannel(VideoChannel& channel, bool visible, bool showPeakBars)
{
    static Logger::ProcLog log("addVideoChannel", Log());
    LOGINFO << channel.getName() << std::endl;

    connections_.push_back(new ChannelConnection(*this, channel, visible, showPeakBars));
    connectionNames_.insert(channel.getName());

    emit channelConnectionsChanged(connectionNames_);

    if (visible) needUpdate();
}

VideoChannel*
Visualizer::removeChannelConnection(int index)
{
    static Logger::ProcLog log("removeChannelConnection", Log());
    LOGINFO << index << std::endl;
    Q_ASSERT(index >= 0 && index < connections_.size());
    ChannelConnection* connection = connections_.takeAt(index);
    VideoChannel* channel = &connection->getChannel();
    connectionNames_.remove(channel->getName());
    emit channelConnectionsChanged(connectionNames_);
    needUpdate();
    delete connection;
    return channel;
}

void
Visualizer::setCurrentView(const ViewSettings& view)
{
    static Logger::ProcLog log("setCurrentView", Log());
    if (view != viewStack_.back()) {
        viewStack_.back() = view;
        updateTransform();
    }
}

void
Visualizer::dupView()
{
    static Logger::ProcLog log("dupView", Log());
    LOGINFO << viewStack_.size() << std::endl;
    viewStack_.push_back(viewStack_.back());
}

void
Visualizer::popView()
{
    static Logger::ProcLog log("popView", Log());
    LOGINFO << viewStack_.size() << std::endl;
    if (canPopView()) {
        ViewSettings last(viewStack_.back());
        viewStack_.pop_back();
        viewStack_.back().setShowingRanges(last.isShowingRanges());
        viewStack_.back().setShowingVoltages(last.isShowingVoltages());
        updateTransform();
    }
}

void
Visualizer::popAllViews()
{
    static Logger::ProcLog log("popAllViews", Log());
    ViewSettings last(viewStack_.back());
    ViewSettings view(viewStack_[0]);
    viewStack_.clear();
    viewStack_.push_back(view);
    viewStack_.back().setShowingRanges(last.isShowingRanges());
    viewStack_.back().setShowingVoltages(last.isShowingVoltages());
    updateTransform();
}

void
Visualizer::updateTransform()
{
    static Logger::ProcLog log("updateTransform", Log());
    const ViewBounds& viewRect(getCurrentView().getBounds());

    double dx = viewRect.getWidth();
    double m11 = (width() - 1.0) / dx;
    dx = -viewRect.getXMin() * m11;

    double dy = viewRect.getHeight();
    double m22 = -(height() - 1.0) / dy;
    dy = height() - 1.0 - viewRect.getYMin() * m22;

    transform_.setMatrix(m11, 0.0, 0.0, m22, dx, dy);
    inverseTransform_ = transform_.inverted();

    LOGDEBUG << "m11: " << m11 << " dx: " << dx << std::endl;
    LOGDEBUG << "m22: " << m22 << " dy: " << dy << std::endl;

    needUpdate();
    emit transformChanged();
}

void
Visualizer::resizeEvent(QResizeEvent* event)
{
    static Logger::ProcLog log("resizeEvent", Log());
    LOGINFO << width() << ' ' << height() << std::endl;
    Super::resizeEvent(event);
    updateTransform();
}

void
Visualizer::showEvent(QShowEvent* event)
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
Visualizer::hideEvent(QHideEvent* event)
{
    static Logger::ProcLog log("hideEvent", Log());
    LOGINFO << std::endl;
    if (updateTimer_.isActive()) { updateTimer_.stop(); }
    Super::hideEvent(event);
}

void
Visualizer::timerEvent(QTimerEvent* event)
{
    static Logger::ProcLog log("timerEvent", Log());

    if (event->timerId() == updateTimer_.timerId()) {
        event->accept();
        if (underMouse()) {
            if (!viewChanger_) {
                Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
                if (mods == Qt::ShiftModifier) {
                    setCursor(Qt::OpenHandCursor);
                } else {
                    setCursor(Qt::CrossCursor);
                }
            }

            QPoint newMouse = mapFromGlobal(QCursor::pos());
            if (newMouse != mouse_) {
                mouse_ = newMouse;
                if (viewChanger_) { viewChanger_->mouseMoved(mouse_); }

                emitPointerMoved();
            }
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
Visualizer::emitPointerMoved()
{
    qreal x, y;
    inverseTransform_.map(qreal(mouse_.x()), qreal(mouse_.y()), &x, &y);
    emit pointerMoved(mouse_, QPointF(x, y));
}

void
Visualizer::paintEvent(QPaintEvent* event)
{
    static Logger::ProcLog log("paintEvent", Log());
    LOGINFO << std::endl;

    QPainter painter(this);
    if (!background_.isNull()) painter.drawPixmap(0, 0, background_);

    double xMin = inverseTransform_.map(QPointF(0, 0)).x();
    double xMax = inverseTransform_.map(QPointF(width(), 0)).x();

    int firstGate = 0;
    int lastGate = 0;

    bool showRanges = viewStack_.back().isShowingRanges();
    bool showVoltages = viewStack_.back().isShowingVoltages();

    if (!showRanges) {
        firstGate = static_cast<int>(xMin);
        lastGate = static_cast<int>(xMax);
    }

    // NOTE: most of this should get moved into a routine in ChannelConnection.
    //
    for (int channel = connections_.size() - 1; channel >= 0; --channel) {
        LOGDEBUG << "channel: " << channel << std::endl;

        ChannelConnection& connection(*getChannelConnection(channel));
        VideoChannel& videoChannel(connection.getChannel());

        Messages::PRIMessage::Ref lastMsg = connection.lastRendered_;
        Messages::PRIMessage::Ref newMsg = historyPosition_->getMessage(connection.historySlot_);

        // If frozen or no new data, work with the last rendered message.
        //
        if (connection.frozen_ || !newMsg) newMsg = lastMsg;

        // Don't do anything if there is no data or the channel is not visible.
        //
        if (!newMsg || !connection.visible_) continue;

        // Generate data only if the old and new messages differ or if the cached plot points vector is empty.
        //
        std::vector<QPointF>& plotPoints(connection.plotPoints_);
        if (newMsg != lastMsg || plotPoints.empty()) {
            connection.lastRendered_ = newMsg;

            Messages::Video::Ref video = boost::dynamic_pointer_cast<Messages::Video>(newMsg);

            plotPoints.clear();

            if (!showRanges) {
                firstGate = static_cast<int>(xMin);
            } else {
                double rangeFactor = video->getRangeFactor();
                double minRange = video->getRangeMin();
                firstGate = static_cast<int>(::floor((xMin - minRange) / rangeFactor) - 1);
                lastGate = static_cast<int>(::ceil((xMax - minRange) / rangeFactor) + 1);
            }

            if (firstGate < 0) firstGate = 0;

            if (firstGate >= static_cast<int>(video->size() - 1)) continue;

            if (lastGate <= firstGate) continue;

            LOGDEBUG << "firstGate: " << firstGate << " lastGate: " << lastGate << " viewWidth: " << width()
                     << std::endl;

            Messages::Video::const_iterator pos = video->begin();
            Messages::Video::const_iterator end = video->end();
            if (lastGate < int(video->size())) end = video->begin() + lastGate + 1;

            pos += firstGate;

            if (showRanges) {
                if (showVoltages) {
                    while (pos < end) {
                        qreal value = videoChannel.getVoltageForSample(*pos);
                        plotPoints.push_back(transform_.map(QPointF(video->getRangeAt(pos), value)));
                        ++pos;
                    }
                } else {
                    while (pos < end) {
                        plotPoints.push_back(transform_.map(QPointF(video->getRangeAt(pos), *pos)));
                        ++pos;
                    }
                }
            } else {
                if (showVoltages) {
                    while (pos < end) {
                        qreal value = videoChannel.getVoltageForSample(*pos);
                        plotPoints.push_back(transform_.map(QPointF(firstGate++, value)));
                        ++pos;
                    }
                } else {
                    while (pos < end) {
                        plotPoints.push_back(transform_.map(QPointF(firstGate++, *pos)));
                        ++pos;
                    }
                }
            }
        }

        painter.setPen(connection.color_);
        painter.drawPolyline(&plotPoints[0], plotPoints.size());

        // Draw the peak bars if visible.
        //
        if (!historyPosition_->isViewingPast() && connection.isReallyShowingPeakBars()) {
            // Recalculate peak bar geometry if the view transform has changed at all.
            //
            if (connection.peakBarRenderer_.isDirty()) {
                connection.peakBarRenderer_.calculateGeometry(transform_, newMsg, showRanges, showVoltages);
            }

            connection.peakBarRenderer_.render(painter);
        }

    } // end for
}

void
Visualizer::mousePressEvent(QMouseEvent* event)
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
Visualizer::mouseReleaseEvent(QMouseEvent* event)
{
    static Logger::ProcLog log("mouseReleaseEvent", Log());
    Super::mouseReleaseEvent(event);
    if (viewChanger_) {
        viewChanger_->finished(event->pos());
        delete viewChanger_;
        viewChanger_ = 0;
        if (event->modifiers() == Qt::ShiftModifier)
            setCursor(Qt::OpenHandCursor);
        else
            setCursor(Qt::CrossCursor);
    }
}

void
Visualizer::keyPressEvent(QKeyEvent* event)
{
    static Logger::ProcLog log("keyPressEvent", Log());

    if (viewChanger_) {
        if (event->key() == Qt::Key_Escape && event->modifiers() == 0) {
            delete viewChanger_;
            viewChanger_ = 0;
            event->accept();
            setCursor(Qt::CrossCursor);
        }

        return;
    }

    if (event->key() == Qt::Key_C) {
        centerAtCursor();
        event->accept();
    } else if (event->key() == Qt::Key_Shift) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }

    Super::keyPressEvent(event);
}

void
Visualizer::keyReleaseEvent(QKeyEvent* event)
{
    static Logger::ProcLog log("keyReleaseEvent", Log());
    if (event->key() == Qt::Key_Shift) setCursor(Qt::CrossCursor);
    Super::keyPressEvent(event);
}

void
Visualizer::focusInEvent(QFocusEvent* event)
{
    static_cast<DisplayView*>(parent())->setActiveDisplayView();
    Super::focusInEvent(event);
}

void
Visualizer::zoom(const QPoint& from, const QPoint& to)
{
    static Logger::ProcLog log("zoom", Log());
    LOGINFO << from.x() << ',' << from.y() << ' ' << to.x() << ',' << to.y() << std::endl;

    // Convert the mouse down point and the mouse up point to real-world values. Set up the bounds so that
    // width()/heigth() is always positive.
    //
    QPointF f(inverseTransform_.map(QPointF(from)));
    QPointF t(inverseTransform_.map(QPointF(to)));
    viewStack_.back().setBounds(
        ViewBounds(std::min(f.x(), t.x()), std::max(f.x(), t.x()), std::min(f.y(), t.y()), std::max(f.y(), t.y())));
    updateTransform();
    update();
}

void
Visualizer::pan(const QPoint& from, const QPoint& to)
{
    static Logger::ProcLog log("pan", Log());
    LOGINFO << from.x() << ',' << from.y() << ' ' << to.x() << ',' << to.y() << std::endl;
    QPointF f(inverseTransform_.map(QPointF(from)));
    QPointF t(inverseTransform_.map(QPointF(to)));
    QRectF fromTo(QRectF(f, QSizeF(t.x() - f.x(), f.y() - t.y())));
    LOGINFO << fromTo.width() << ' ' << fromTo.height() << std::endl;
    ViewBounds bounds(viewStack_.back().getBounds());
    bounds.translate(f.x() - t.x(), f.y() - t.y());
    viewStack_.back().setBounds(bounds);
    updateTransform();
    update();
}

void
Visualizer::raiseChannelConnection(int index)
{
    if (index >= connections_.size() - 1) return;
    ChannelConnection* connection = connections_[index];
    connections_.erase(connections_.begin() + index);
    connections_.insert(index + 1, connection);
    needUpdate();
}

void
Visualizer::lowerChannelConnection(int index)
{
    if (index < 1) return;
    ChannelConnection* connection = connections_[index];
    connections_.erase(connections_.begin() + index);
    connections_.insert(index - 1, connection);
    needUpdate();
}

int
Visualizer::findVideoChannel(VideoChannel* channel) const
{
    for (int index = 0; index < connections_.size(); ++index) {
        if (&connections_[index]->getChannel() == channel) return index;
    }
    return -1;
}

void
Visualizer::setShowGrid(bool state)
{
    showGrid_ = state;
    needUpdate();
    emit transformChanged();
}

void
Visualizer::setShowPeakBars(bool state)
{
    if (state != showPeakBars_) {
        showPeakBars_ = state;
        needUpdate();
    }
}

void
Visualizer::centerAtCursor()
{
    QPoint pos(mapFromGlobal(QCursor::pos()));
    pos = inverseTransform_.map(pos);

    ViewSettings newView(viewStack_.back());
    ViewBounds bounds(newView.getBounds());
    bounds.translate(pos.x() - newView.getBounds().getWidth() / 2, pos.y() - newView.getBounds().getHeight() / 2);
    newView.setBounds(bounds);
    viewStack_.push_back(newView);
    pos = QPoint(width() / 2, height() / 2);
    QCursor::setPos(mapToGlobal(pos));
    updateTransform();
}

void
Visualizer::swapViews()
{
    size_t other = viewStack_.size() - 2;
    ViewSettings view(viewStack_.back());
    viewStack_.back() = viewStack_[other];
    viewStack_[other] = view;
    updateTransform();
}

void
Visualizer::setShowingRanges(bool state)
{
    viewStack_.back().setShowingRanges(state);
    updateTransform();
    emitPointerMoved();
}

void
Visualizer::setShowingVoltages(bool state)
{
    viewStack_.back().setShowingVoltages(state);
    updateTransform();
    emitPointerMoved();
}

ChannelConnection*
Visualizer::getChannelConnection(const QString& name) const
{
    foreach (ChannelConnection* connection, connections_) {
        if (connection->getChannel().getName() == name) return connection;
    }

    return 0;
}
