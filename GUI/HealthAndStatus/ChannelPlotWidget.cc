#include <cmath>
#include <numeric>

#include "QtGui/QAction"

#include "GUI/LogUtils.h"
#include "GUI/MessageList.h"
#include "Messages/Video.h"
#include "Utils/RunningMedian.h"

#include "App.h"
#include "ChannelConnection.h"
#include "ChannelPlotSettings.h"
#include "ChannelPlotWidget.h"
#include "ConfigurationWindow.h"

using namespace SideCar::GUI::HealthAndStatus;

Logger::Log&
ChannelPlotWidget::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("hands.ChannelPlotWidget");
    return log_;
}

QString
ChannelPlotWidget::GetFormattedValue(double value)
{
    return QString::number(value, 'f', 1);
}

QString
ChannelPlotWidget::getFormattedDelta(double value) const
{
    value -= expectedValue_;
    return QString(value >= 0 ? '+' : '-') +
	GetFormattedValue(::fabs(value));
}

ChannelPlotWidget::ChannelPlotWidget(ChannelConnection* channelConnection,
                                     QWidget* parent)
    : Super(parent), Ui::ChannelPlotWidget(),
      settings_(channelConnection->getSettings()), sampleMedian_(0),
      algorithm_(kAverage), messageCounter_(0), dropCount_(0),
      lastSequenceNumber_(uint32_t(-1)), timerId_(0)
{
    Logger::ProcLog log("ChannelPlotWidget", Log());
    setupUi(this);
    setObjectName(QString("ChannelPlotWidget ") +
                  channelConnection->getName());
    setStyleSheet("border: 1px solid #202020");

    channelName_->setText(channelConnection->getName());

    connect(channelConnection, SIGNAL(connected()),
            SLOT(connected()));
    connect(channelConnection, SIGNAL(disconnected()),
            SLOT(disconnected()));
    connect(channelConnection, SIGNAL(incoming(const MessageList&)),
            SLOT(processIncoming(const MessageList&)));

    showConnected(channelConnection->isConnected());

    messageDecimation_ = settings_->getMessageDecimation();
    sampleStartIndex_ = settings_->getSampleStartIndex();
    sampleCount_ = settings_->getSampleCount();
    setSamplesColor(settings_->getSamplesColor());
    setExpectedVariance(settings_->getExpectedVariance());
    setExpected(settings_->getExpected());
    setAlgorithm(settings_->getAlgorithm());

    settings_->connectToWidget(this);

    clearAll();
}

ChannelPlotWidget::~ChannelPlotWidget()
{
    Logger::ProcLog log("~ChannelPlotWidget", Log());
    LOGINFO << objectName() << std::endl;
    delete sampleMedian_;
    sampleMedian_ = 0;
}

void
ChannelPlotWidget::showConnected(bool connected)
{
    Logger::ProcLog log("showConnected", Log());
    LOGERROR << objectName() << ' ' << connected << std::endl;
    QColor color(connected ? Qt::green : Qt::red);
    QPalette p(palette());
    p.setColor(QPalette::Active, QPalette::WindowText, color);
    p.setColor(QPalette::Inactive, QPalette::WindowText, color);
    p.setColor(QPalette::Active, QPalette::Text, color);
    p.setColor(QPalette::Inactive, QPalette::Text, color);
    p.setColor(QPalette::Active, QPalette::HighlightedText, color);
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, color);
    channelName_->setPalette(p);
    dropsLabel_->setPalette(p);
    drops_->setPalette(p);
    min_->setPalette(p);
    max_->setPalette(p);
    expected_->setPalette(p);
    sampleLabel_->setPalette(p);
    sample_->setPalette(p);
}

void
ChannelPlotWidget::updateValueColor(QLabel* label, bool tooBig)
{
    QColor color(tooBig ? Qt::yellow : Qt::green);
    QPalette p(palette());
    p.setColor(QPalette::Active, QPalette::WindowText, color);
    p.setColor(QPalette::Inactive, QPalette::WindowText, color);
    p.setColor(QPalette::Active, QPalette::Text, color);
    p.setColor(QPalette::Inactive, QPalette::Text, color);
    p.setColor(QPalette::Active, QPalette::HighlightedText, color);
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, color);
    label->setPalette(p);
}

void
ChannelPlotWidget::connected()
{
    Logger::ProcLog log("connected", Log());
    LOGERROR << objectName() << std::endl;
    showConnected(true);
    if (! timerId_)
	timerId_ = startTimer(500);
}

void
ChannelPlotWidget::disconnected()
{
    Logger::ProcLog log("disconnected", Log());
    LOGERROR << objectName() << std::endl;
    showConnected(false);
    if (timerId_) {
	killTimer(timerId_);
	timerId_ = 0;
    }
}

void
ChannelPlotWidget::processIncoming(const MessageList& msgs)
{
    static Logger::ProcLog log("processIncoming", Log());
    LOGINFO << std::endl;

    MessageList::const_iterator pos = msgs.begin();
    MessageList::const_iterator end = msgs.end();
    while (pos != end) {
	Messages::Video::Ref msg =
	    boost::dynamic_pointer_cast<Messages::Video>(*pos++);

	uint32_t sequenceNumber = msg->getMessageSequenceNumber();

	if (lastSequenceNumber_ == uint32_t(-1) ||
            sequenceNumber < lastSequenceNumber_)
	    ;
	else if (sequenceNumber > lastSequenceNumber_ + 1) {
	    dropCount_ += (sequenceNumber - lastSequenceNumber_ - 1);
	    drops_->setNum(dropCount_);
	}

	lastSequenceNumber_ = sequenceNumber;

	int size = msg->size();
	int start = sampleStartIndex_;
	int count = sampleCount_;

	if (start < 0) {
	    start += size;
	    if (start < 0)
		start = 0;
	}

	if (start >= size)
	    continue;

	if (start + count > size)
	    count = size - start;

	Messages::Video::const_iterator pos = msg->begin() + start;
	Messages::Video::const_iterator end = msg->begin() + start + count;
	double sample = std::accumulate(pos, end, 0) / double(count);

	if (! sampleMedian_)
	    sampleMedian_ = new Utils::RunningMedian(
		settings_->getRunningMedianWindowSize(), expectedValue_);

	double median = sampleMedian_->addValue(sample);

	if (++messageCounter_ >= messageDecimation_) {
	    messageCounter_ = 0;
	    LOGDEBUG << "sample: " << sample << " median: " << median
		     << std::endl;
	    if (algorithm_ == 1)
		plot_->addSample(median);
	    else
		plot_->addSample(sampleMedian_->getEstimatedMeanValue());
	}
    }
}

void
ChannelPlotWidget::clearAll()
{
    delete sampleMedian_;
    sampleMedian_ = 0;
    clearDrops();
    plot_->clear();
    if (timerId_) 
	timerEvent(0);
}

void
ChannelPlotWidget::clearDrops()
{
    lastSequenceNumber_ = uint32_t(-1);
    drops_->setNum(0);
}

void
ChannelPlotWidget::setMessageDecimation(int value)
{
    messageDecimation_ = value;
}

void
ChannelPlotWidget::setSampleStartIndex(int value)
{
    sampleStartIndex_ = value;
}

void
ChannelPlotWidget::setSampleCount(int value)
{
    sampleCount_ = value;
}

void
ChannelPlotWidget::setRunningMedianWindowSize(int value)
{
    if (sampleMedian_)
	sampleMedian_->setWindowSize(value, sampleMedian_->getMedianValue());
}

void
ChannelPlotWidget::setAlgorithm(int value)
{
    algorithm_ = value;
    plot_->clear();
}

void
ChannelPlotWidget::setSamplesColor(const QColor& color)
{
    Logger::ProcLog log("setSamplesColor", Log());
    LOGERROR << objectName() << " color: " << color.name() << std::endl;
    plot_->setSamplesColor(color);
}

void
ChannelPlotWidget::setExpectedVariance(double value)
{
    plot_->setExpectedVariance(value);
    if (timerId_) {
	updateValueColor(min_, minValue_ < expectedValue_ - value);
	updateValueColor(max_, maxValue_ > expectedValue_ + value);
    }
}

void
ChannelPlotWidget::setExpected(double value)
{
    expectedValue_ = value;
    expected_->setText(GetFormattedValue(value));
    plot_->setExpected(value);
    min_->setText(getFormattedDelta(plot_->getMinimumValue()));
    max_->setText(getFormattedDelta(plot_->getMaximumValue()));
    if (timerId_) {
	double var = settings_->getExpectedVariance();
	updateValueColor(min_, minValue_ < expectedValue_ - var);
	updateValueColor(max_, maxValue_ > expectedValue_ + var);
    }
}

void
ChannelPlotWidget::showEvent(QShowEvent* event)
{
    ;
}

void
ChannelPlotWidget::closeEvent(QCloseEvent* event)
{
    if (timerId_) {
	killTimer(timerId_);
	timerId_ = 0;
    }
}

void
ChannelPlotWidget::timerEvent(QTimerEvent* event)
{
    if (! event || event->timerId() == timerId_) {
	double variance = settings_->getExpectedVariance();
	double value = plot_->getMinimumValue();
	if (minValue_ != value) {
	    minValue_ = value;
	    min_->setText(getFormattedDelta(value));
	    updateValueColor(min_, minValue_ < expectedValue_ - variance);
	}

	value = plot_->getMaximumValue();
	if (maxValue_ != value) {
	    maxValue_ = value;
	    max_->setText(getFormattedDelta(value));
	    updateValueColor(max_, maxValue_ > expectedValue_ + variance);
	}

	value = plot_->getSampleValue();
	if (sampleValue_ != value) {
	    sampleValue_ = value;
	    sample_->setText(getFormattedDelta(value));
	    updateValueColor(sample_, value < (expectedValue_ - variance) ||
                             value > (expectedValue_ + variance));
	}
    }
}

void
ChannelPlotWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QAction* action =
	App::GetApp()->getConfigurationWindow()->getShowHideAction();
    if (! action->isChecked())
	action->trigger();
}

double
ChannelPlotWidget::getEstimatedValue() const
{
    return plot_->getAverageSampleValue();
}

