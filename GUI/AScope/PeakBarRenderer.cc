#include "QtGui/QMatrix"
#include "QtGui/QPainter"

#include "GUI/LogUtils.h"

#include "App.h"
#include "ChannelConnection.h"
#include "PeakBarCollection.h"
#include "PeakBarRenderer.h"
#include "PeakBarSettings.h"
#include "VideoChannel.h"

using namespace SideCar::GUI::AScope;

Logger::Log&
PeakBarRenderer::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.PeakBarRenderer");
    return log_;
}

PeakBarRenderer::PeakBarRenderer(ChannelConnection& channelConnection) :
    Super(0), channelConnection_(channelConnection), model_(channelConnection.getChannel().getPeakBars()), cache_(),
    lines_(), rects_(), barSize_(-1, -1), origin_(-1, -1), width_(-1), fading_(false), enabled_(true), dirty_(true),
    allDirty_(true), transformDirty_(true)
{
    PeakBarSettings& settings = App::GetApp()->getPeakBarSettings();
    width_ = settings.getWidth();
    fading_ = settings.isFading();
    enabled_ = settings.isEnabled();

    connect(&settings, SIGNAL(widthChanged(int)), SLOT(widthChanged(int)));
    connect(&settings, SIGNAL(fadingChanged(bool)), SLOT(fadingChanged(bool)));
    connect(&settings, SIGNAL(enabledChanged(bool)), SLOT(enabledChanged(bool)));

    connect(&model_, SIGNAL(gateTransformChanged()), SLOT(gateTransformChanged()));
    connect(&model_, SIGNAL(barCountChanged(int)), SLOT(barCountChanged(int)));
    connect(&model_, SIGNAL(barValuesChanged(const QVector<int>&)), SLOT(barValuesChanged(const QVector<int>&)));
}

void
PeakBarRenderer::unfreeze()
{
    // Going from a frozen to not-frozen state. Fetch the latest peak bar information, and regenerate rendering
    // information.
    //
    cache_ = model_.getPeakBars();
    dirtyCache(true);
}

void
PeakBarRenderer::calculateGeometry(const QMatrix& transform, const Messages::PRIMessage::Ref& msg, bool showRanges,
                                   bool showVoltages)
{
    static Logger::ProcLog log("calculateGeometry", Log());
    LOGINFO << msg << std::endl;

    double x = 0.0;
    double width = width_;

    if (showRanges) {
        x = msg->getRangeMin();
        width = msg->getRangeFactor() * width;
    }

    // See if we need to recalculate bar dimensions due to transform or bar width changes.
    //
    if (transformDirty_) {
        transformDirty_ = false;
        allDirty_ = true;

        // Calculate the bar dimensions, translating from real-world values.
        //
        origin_ = transform.map(QPointF(0, 0));
        qreal barWidth, barHeight;
        transform.map(width, 0, &barWidth, &barHeight);
        barWidth -= origin_.x();
        barHeight = barWidth * 0.6180339888 / 2.0; // golden-ratio / 2

        // If the bar height is too small for a rectangle fill, force to be a line.
        //
        if (barHeight < 1.5)
            barHeight = 1.0;
        else if (barHeight > 4.0)
            barHeight = 4.0;

        barSize_ = QSizeF(barWidth, barHeight);
    }

    VideoChannel& channel(channelConnection_.getChannel());

    bool usingLines = barSize_.height() == 1.0;
    for (int index = 0; index < cache_.size(); ++index) {
        if (allDirty_ || (!usingLines && rects_[index].isNull()) || (usingLines && lines_[index].isNull())) {
            int sample = cache_[index].getValue();
            LOGDEBUG << "index: " << index << " sample: " << sample << std::endl;

            qreal value = showVoltages ? channel.getVoltageForSample(sample) : sample;

            LOGDEBUG << "value: " << value << std::endl;

            QPointF from = transform.map(QPointF(x, value));
            if (usingLines) {
                QPointF to(from);
                to.rx() += barSize_.width();
                lines_[index] = QLineF(from, to);
            } else {
                from.ry() -= barSize_.height();
                rects_[index] = QRectF(from, barSize_);
            }
        }

        x += width;
    }

    dirty_ = false;
    allDirty_ = false;
}

void
PeakBarRenderer::render(QPainter& painter)
{
    static Logger::ProcLog log("render", Log());
    LOGINFO << std::endl;

    if (dirty_) { LOGDEBUG << "there are dirty elements!" << std::endl; }

    QPen pen(painter.pen());
    pen.setWidth(0);
    QColor color(pen.color());

    if (barSize_.height() == 1.0) {
        if (!fading_) {
            painter.drawLines(lines_);
        } else {
            for (int index = 0; index < cache_.size(); ++index) {
                if (lines_[index].isNull()) {
                    LOGERROR << "invalid line at index " << index << std::endl;
                } else {
                    color.setAlphaF(cache_[index].getDecay());
                    pen.setColor(color);
                    painter.setPen(pen);
                    painter.drawLine(lines_[index]);
                }
            }
        }
    } else {
        for (int index = 0; index < cache_.size(); ++index) {
            if (rects_[index].isNull()) {
                LOGERROR << "invalid rect at index " << index << std::endl;
            } else {
                if (fading_) color.setAlphaF(cache_[index].getDecay());
                painter.fillRect(rects_[index], color);
            }
        }
    }
}

void
PeakBarRenderer::gateTransformChanged()
{
    transformDirty_ = true;
    dirtyCache(true);
}

void
PeakBarRenderer::barCountChanged(int size)
{
    lines_.resize(size);
    rects_.resize(size);
    dirtyCache(true);
}

void
PeakBarRenderer::barValuesChanged(const QVector<int>& changed)
{
    static Logger::ProcLog log("barValuesChanged", Log());
    LOGINFO << std::endl;

    static const QLineF nullLine_;
    static const QRectF nullRect_;

    if (!channelConnection_.isReallyFrozen()) {
        // Only cause an update() if there are value changes to any of the bars.
        //
        if (!changed.empty()) dirty_ = true;

        // Copy over all value and aging information for the bars. Aging information is always used by the
        // render() routine, even if the dirty_ flag is not set.
        //
        cache_ = model_.getPeakBars();
        if (cache_.size() != lines_.size()) { barCountChanged(cache_.size()); }

        // For each element that was changed, zap its cached QLineF and QRectF representations so that a future
        // update() call will regenerate them.
        //
        for (int index = 0; index < changed.size(); ++index) {
            int pos = changed[index];
            LOGDEBUG << "index " << pos << " changed" << std::endl;
            lines_[pos] = nullLine_;
            rects_[pos] = nullRect_;
        }
    }
}

void
PeakBarRenderer::widthChanged(int value)
{
    if (width_ != value) {
        width_ = value;
        gateTransformChanged();
    }
}

void
PeakBarRenderer::fadingChanged(bool value)
{
    fading_ = value;
}

void
PeakBarRenderer::enabledChanged(bool value)
{
    enabled_ = value;
}

void
PeakBarRenderer::dirtyCache(bool allDirty)
{
    allDirty_ = allDirty;
    dirty_ = true;
}
