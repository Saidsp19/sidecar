#include <algorithm>
#include <numeric>

#include "QtGui/QPainter"

#include "GUI/LogUtils.h"
#include "Utils/RunningMedian.h"

#include "ChannelPlotSettings.h"
#include "PlotDataPointDeque.h"
#include "SamplePlotWidget.h"

using namespace SideCar::GUI::HealthAndStatus;

static int kTickSpan = 150; // Show a tick every kTickSpan pixels

Logger::Log&
SamplePlotWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("hands.SamplePlotWidget");
    return log_;
}

SamplePlotWidget::SamplePlotWidget(QWidget* parent) :
    Super(parent), plotStats_(0), plotDataPoints_(), samplePoints_(), sample_(0.0), expectedVariance_(1.0),
    expected_(0.0), samplesColor_(Qt::white)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

SamplePlotWidget::~SamplePlotWidget()
{
    delete plotStats_;
    plotStats_ = 0;
}

void
SamplePlotWidget::clear()
{
    sample_ = expected_;

    delete plotStats_;
    plotStats_ = 0;
    plotDataPoints_.clear();

    if (width() > 1) {
        size_t w = width();
        while (w < samplePoints_.size()) samplePoints_.pop_back();

        while (w > samplePoints_.size()) samplePoints_.push_back(QPoint(samplePoints_.size(), 0));
    }

    update();
}

struct StatsFiller {
    StatsFiller(Utils::RunningMedian& stats) : stats_(stats) {}

    void operator()(const PlotDataPoint& data) { stats_.addValue(data.sample); }

    Utils::RunningMedian& stats_;
};

void
SamplePlotWidget::resizePlot()
{
    Logger::ProcLog log("resizePlot", Log());

    size_t newSize = width();
    size_t oldSize = samplePoints_.size();

    LOGINFO << "oldSize: " << oldSize << " newSize: " << newSize << std::endl;

    if (newSize == oldSize) return;

    if (plotStats_) plotStats_->setWindowSize(newSize, sample_);

    if (newSize < oldSize) {
        // Need to make containers smaller
        //
        samplePoints_.resize(newSize);

        // Remove oldest samples to keep our plotDataPoints_ container the same size as the others.
        //
        while (newSize < plotDataPoints_.size()) plotDataPoints_.pop_back();
    } else if (newSize > oldSize) {
        // Need to make containers larger
        //
        do {
            samplePoints_.push_back(QPoint(oldSize++, 0));
        } while (newSize > oldSize);
    }

    if (plotStats_) {
        // When the Utils::RunningMedian window size changes, it forgets what data it was holding, so we need to
        // add them back. Limitation? Bug?
        //
        std::for_each(plotDataPoints_.rbegin(), plotDataPoints_.rend(), StatsFiller(*plotStats_));
    }

    update();
}

void
SamplePlotWidget::setExpectedVariance(double value)
{
    if (value < 0.001) value = 0.001;
    expectedVariance_ = value;
    update();
}

void
SamplePlotWidget::setSamplesColor(const QColor& color)
{
    samplesColor_ = color;
    update();
}

void
SamplePlotWidget::resizeEvent(QResizeEvent* event)
{
    Logger::ProcLog log("resizeEvent", Log());
    LOGINFO << "width: " << width() << std::endl;
    if (width() > 1) resizePlot();
}

void
SamplePlotWidget::showEvent(QShowEvent* event)
{
    Logger::ProcLog log("resizeEvent", Log());
    LOGINFO << "width: " << width() << std::endl;
    if (width() > 1) resizePlot();
}

void
SamplePlotWidget::addSample(double sample)
{
    if (!plotStats_) plotStats_ = new Utils::RunningMedian(width(), sample);

    plotStats_->addValue(sample);
    sample_ = sample;
    plotDataPoints_.push_front(PlotDataPoint(sample));

    size_t limit = width();
    while (plotDataPoints_.size() > limit) plotDataPoints_.pop_back();

    update();
}

void
SamplePlotWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), Qt::black);

    // Default scaling will be expected_ +/- expectedVariance. However, if our min or max value is outside this
    // range, then the scaling will expand to to it.
    //
    double span = expectedVariance_ * 2;
    double min = expected_ - expectedVariance_;
    if (plotStats_) {
        double valueMax = plotStats_->getMaximumValue();
        if (expected_ > valueMax) valueMax = expected_;
        double valueMin = plotStats_->getMinimumValue();
        if (expected_ < valueMin) valueMin = expected_;
        double valueSpan = plotStats_->getMaximumValue() - plotStats_->getMinimumValue();
        if (valueSpan > span) {
            span = valueSpan;
            min = plotStats_->getMinimumValue();
        }
    }

    // Calculate conversion parameters, scale sample values into pixel Y positions, and draw lines connecting
    // the sample values.
    //
    double offset = (height() - 1);
    double scale = offset / span;
    offset += min * scale;
    size_t count = plotDataPoints_.size();
    for (size_t index = 0; index < count; ++index) {
        double value = offset - scale * plotDataPoints_[index].sample;
        samplePoints_[index].setY(static_cast<int>(::rint(value)));
    }

    // Draw a line showing the expected (nominal) value in the plot.
    //
    int expected = static_cast<int>(::rint(offset - scale * expected_));
    painter.setPen(Qt::gray);
    painter.drawLine(0, expected, width() - 1, expected);

    // Generate tick marks to show elapsed time in seconds on the plot.
    //
    painter.setFont(font());
    int x = kTickSpan;
    Time::TimeStamp now = Time::TimeStamp::Now();
    int numDataPoints = plotDataPoints_.size();
    while (x < width() && x < numDataPoints) {
        double seconds = (now - plotDataPoints_[x].when).asDouble();
        QString text = QString("%1s").arg(int(::rint(seconds)));
        QRect rect(painter.boundingRect(0, 0, width(), height(), 0, text));
        int v = height() - rect.height() + 1;
        painter.drawText(x - rect.width() / 2, v, rect.width(), rect.height(), 0, text);
        painter.drawLine(x, expected + 10, x, expected - 10);
        x += kTickSpan;
    }

    painter.setPen(samplesColor_);
    painter.drawPolyline(&samplePoints_[0], count);
}

void
SamplePlotWidget::setExpected(double value)
{
    expected_ = value;
    update();
}

struct Adder {
    PlotDataPoint operator()(const PlotDataPoint& lhs, const PlotDataPoint& rhs) const
    {
        return PlotDataPoint(lhs.sample + rhs.sample);
    }
};

double
SamplePlotWidget::getAverageSampleValue() const
{
    if (plotDataPoints_.empty()) return expected_;
    PlotDataPoint value(std::accumulate(plotDataPoints_.begin(), plotDataPoints_.end(), PlotDataPoint(0.0), Adder()));
    return value.sample / plotDataPoints_.size();
}

double
SamplePlotWidget::getMinimumValue() const
{
    return plotStats_ ? plotStats_->getMinimumValue() : expected_;
}

double
SamplePlotWidget::getMaximumValue() const
{
    return plotStats_ ? plotStats_->getMaximumValue() : expected_;
}
