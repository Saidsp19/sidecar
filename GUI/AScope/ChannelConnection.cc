#include "GUI/LogUtils.h"

#include "History.h"
#include "HistoryPosition.h"
#include "PeakBarRenderer.h"
#include "VideoChannel.h"
#include "Visualizer.h"

using namespace SideCar;
using namespace SideCar::GUI::AScope;

Logger::Log&
ChannelConnection::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.ChannelConnection");
    return log_;
}

ChannelConnection::ChannelConnection(Visualizer& visualizer,
                                     VideoChannel& channel, bool visible,
                                     bool showPeakBars)
    : QObject(), channel_(channel), visualizer_(visualizer),
      peakBarRenderer_(*this), lastRendered_(), plotPoints_(),
      historySlot_(channel.displayAdded()), color_(channel.getColor()),
      visible_(visible), showPeakBars_(showPeakBars), frozen_(false)
{
    connect(this, SIGNAL(redisplay()), &visualizer, SLOT(update()));
    connect(&channel, SIGNAL(colorChanged(const QColor&)),
            SLOT(setColor(const QColor&)));
    connect(&visualizer, SIGNAL(transformChanged()), SLOT(clearCache()));
    connect(&channel, SIGNAL(sampleToVoltageScalingChanged()),
            SLOT(clearCache()));
}

ChannelConnection::~ChannelConnection()
{
    static Logger::ProcLog log("~ChannelConnection", Log());
    LOGINFO << std::endl;
    channel_.displayDropped();
}

void
ChannelConnection::setVisible(bool state)
{
    if (visible_ != state) {
	visible_ = state;
	emit redisplay();
    }
}

void
ChannelConnection::setShowPeakBars(bool state)
{
    if (state != showPeakBars_) {
	showPeakBars_ = state;
	emit redisplay();
    }
}

void
ChannelConnection::setFrozen(bool state)
{
    if (frozen_ != state) {
	frozen_ = state;
	if (! state)
	    peakBarRenderer_.unfreeze();
	emit redisplay();
    }
}

void
ChannelConnection::setColor(const QColor& color)
{
    if (color_ != color) {
	color_ = color;
	channel_.setColor(color);
	emit redisplay();
    }
}

bool
ChannelConnection::isReallyShowingPeakBars() const
{
    return showPeakBars_ && visualizer_.isShowingPeakBars() &&
	peakBarRenderer_.isEnabled();
}

bool
ChannelConnection::isReallyFrozen() const
{
    return frozen_ || visualizer_.isFrozen();
}

void
ChannelConnection::clearCache()
{
    peakBarRenderer_.gateTransformChanged();
    plotPoints_.clear();
}
