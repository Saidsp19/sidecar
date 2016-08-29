#include "GUI/LogUtils.h"
#include "GUI/MessageList.h"

#include "App.h"
#include "PeakBarCollection.h"
#include "PeakBarSettings.h"

using namespace SideCar;
using namespace SideCar::GUI::AScope;

Logger::Log&
PeakBarCollection::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.PeakBarCollection");
    return log_;
}

PeakBarCollection::PeakBarCollection(QObject* parent)
    : Super(parent), peakBars_(), changed_(), last_(), width_(-1),
      enabled_(false)
{
    PeakBarSettings& settings = App::GetApp()->getPeakBarSettings();

    width_ = settings.getWidth();
    enabled_ = settings.isEnabled();

    connect(&settings, SIGNAL(widthChanged(int)),
            SLOT(widthChanged(int)));
    connect(&settings, SIGNAL(lifeTimeChanged(int)),
            SLOT(lifeTimeChanged(int)));
    connect(&settings, SIGNAL(enabledChanged(bool)),
            SLOT(enabledChanged(bool)));
}

void
PeakBarCollection::widthChanged(int value)
{
    width_ = value;
    adjustBars(true);
}

void
PeakBarCollection::lifeTimeChanged(int value)
{
    adjustBars(true);
}

void
PeakBarCollection::enabledChanged(bool value)
{
    enabled_ = value;
    adjustBars(true);
}

void
PeakBarCollection::update(const Messages::PRIMessage::Ref& msg)
{
    static Logger::ProcLog log("update", Log());
    LOGINFO << std::endl;
    if (! msg) return;
    last_ = boost::dynamic_pointer_cast<Messages::Video>(msg);
    adjustBars(false);
}

void
PeakBarCollection::adjustBars(bool resetBars)
{
    static Logger::ProcLog log("adjustBars", Log());
    LOGINFO << "resetBars: " << resetBars << std::endl;

    if (! last_) {
	LOGDEBUG << "no message" << std::endl;
	return;
    }

    int barsNeeded = int(::floor(double(last_->size() + width_ - 1) /
                                 width_));
    if (barsNeeded != peakBars_.size()) {
	LOGDEBUG << "resizing peakBars_ - " << barsNeeded << std::endl;
 	peakBars_.resize(barsNeeded);
	emit barCountChanged(barsNeeded);
    }

    if (! enabled_)
	return;

    changed_.clear();
    Messages::Video::const_iterator pos = last_->begin();

    for (int index = 0; index < peakBars_.size(); ++index) {

	Messages::Video::const_iterator end = pos + width_;
	if (end > last_->end())
	    end = last_->end();

	if (pos == end) {
	    LOGDEBUG << "no data for bar " << index << std::endl;
	}

	if (peakBars_[index].update(pos, end, resetBars)) {
	    LOGDEBUG << "bar " << index << " has changed" << std::endl;
	    changed_.push_back(index);
	}

	pos = end;
    }

    emit barValuesChanged(changed_);
}
