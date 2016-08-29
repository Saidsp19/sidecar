#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iterator>

#include "IO/MessageManager.h"
#include "Messages/RadarConfig.h"
#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "History.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::PPIDisplay;
using namespace SideCar::Messages;

int History::Entry::extractionsLifeTime_ = 30 * 1000;
int History::Entry::rangeTruthsLifeTime_ = 30 * 1000;
int History::Entry::rangeTruthsMaxTrailLength_ = 10;
int History::Entry::bugPlotsLifeTime_ = 30 * 1000;

static size_t
calculateCapacity(size_t size)
{
    // Find the next power-of-ten multiple of a given value. First, determine the largest power-of-ten that the
    // given value is greater than. Reduce the value by that scaling factor, round up, and rescale to get the
    // new value.
    //
    double scale = ::pow(10, int(::log10(size)));
    return size_t(::ceil(size / scale) * scale);
}

History::Entry::Entry(size_t lastVideoSize, size_t lastBinarySize)
    : video_(), binary_(), extractions_(), rangeTruths_(),
      bugPlots_()
{
    size_t capacity = lastVideoSize ? calculateCapacity(lastVideoSize) : 5000;
    video_.reserve(capacity);
    if (lastBinarySize)
	binary_.reserve(capacity);
    clearAll();
}

PRIMessage::Ref
History::Entry::findByAzimuth(const MessageVector& data, double azimuth)
    const
{
    static const int kMinDelta = 10;

    if (data.empty())
	return PRIMessage::Ref();

    // Obtain shaft encoding for the given azimuth value.
    //
    int wanted = static_cast<int>(
	::rint(azimuth * (RadarConfig::GetShaftEncodingMax() + 1) /
               (M_PI * 2.0)));

    MessageVector::const_iterator low = data.begin();
    int lowShaft = (*low)->getShaftEncoding();
    if (wanted < lowShaft)
	wanted += (RadarConfig::GetShaftEncodingMax() + 1);

    MessageVector::const_iterator high = data.end() - 1;
    int highShaft = (*high)->getShaftEncoding();
    if (highShaft < wanted)
	highShaft += (RadarConfig::GetShaftEncodingMax() + 1);

    while (low < high) {

	MessageVector::const_iterator mid = low + (high - low) / 2;
	const PRIMessage::Ref& msg(*mid);
	int midShaft = msg->getShaftEncoding();

	if (midShaft < lowShaft)
	    midShaft += (RadarConfig::GetShaftEncodingMax() + 1);

	if (wanted == midShaft) {
	    return msg;
	    break;
	}

	if (mid == low) {

	    if (wanted == highShaft)
		return (*high);

	    int deltaLow = wanted - lowShaft;
	    int deltaHigh = highShaft - wanted;
	    if (deltaLow < deltaHigh) {
		if (deltaLow < kMinDelta)
		    return *low;
	    }
	    else {
		if (deltaHigh < kMinDelta)
		    return *high;
	    }
	    break;
	}

	if (wanted < midShaft) {
	    high = mid;
	    highShaft = midShaft;
	}
	else {
	    low = mid;
	    lowShaft = midShaft;
	}
    }

    return PRIMessage::Ref();
}

Video::DatumType
History::Entry::getVideoValue(double azimuth, double range, bool& isValid)
    const
{
    PRIMessage::Ref found(findByAzimuth(video_, azimuth));
    if (! found) {
	isValid = false;
	return Video::DatumType();
    }

    size_t wantedRangeGate = ::rint((range - found->getRangeMin()) /
                                    found->getRangeFactor());
    if (wantedRangeGate >= found->size()) {
	isValid = false;
	return Video::DatumType();
    }

    const Video::Ref video = boost::dynamic_pointer_cast<Video>(found);
    isValid = true;
    return video[wantedRangeGate];
}

BinaryVideo::DatumType
History::Entry::getBinaryValue(double azimuth, double range, bool& isValid)
    const
{
    PRIMessage::Ref found(findByAzimuth(binary_, azimuth));
    if (! found) {
	isValid = false;
	return Video::DatumType();
    }

    size_t wantedRangeGate = ::rint((range - found->getRangeMin()) /
                                    found->getRangeFactor());
    if (wantedRangeGate >= found->size()) {
	isValid = false;
	return Video::DatumType();
    }

    const BinaryVideo::Ref binary =
	boost::dynamic_pointer_cast<BinaryVideo>(found);
    isValid = true;
    return binary[wantedRangeGate];
}

void
History::Entry::clearAll()
{
    clearVideo();
    clearBinary();
    clearExtractions();
    clearRangeTruths();
    clearBugPlots();
}

void
History::Entry::clearVideo()
{
    lastVideoSlot_ = 0;
    video_.clear();
}

void
History::Entry::clearBinary()
{
    lastBinarySlot_ = 0;
    binary_.clear();
}

void
History::Entry::clearExtractions()
{
    extractions_.clear();
}

void
History::Entry::clearRangeTruths()
{
    rangeTruths_.clear();
}

void
History::Entry::clearBugPlots()
{
    bugPlots_.clear();
}

void
History::Entry::freeze()
{
    static Logger::ProcLog log("Entry::freeze", Log());
    LOGINFO << "video size: " << (lastVideoSlot_ + 1)
            << "binary size: " << (lastBinarySlot_ + 1)
            << std::endl;

    // Reduce vectors if their size is larger than the last recorded entry.
    //
    if ((lastVideoSlot_ + 1) < video_.size())
	video_.resize(lastVideoSlot_ + 1);
    if ((lastBinarySlot_ + 1) < binary_.size())
	binary_.resize(lastBinarySlot_ + 1);
}

bool
History::Entry::addVideo(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("Entry::addVideo", Log());

    bool filled = false;
    
    if (video_.empty()) {
	video_.push_back(msg);
	lastVideoSlot_ = 0;
	return filled;
    }

    Messages::PRIMessage::Ref last = video_[lastVideoSlot_];
    if (msg->getShaftEncoding() == last->getShaftEncoding()) {
	video_[lastVideoSlot_] = msg;
    }
    else {
	if (last->getAzimuthStart() > msg->getAzimuthStart() + M_PI) {
	    LOGDEBUG << "detected full entry - " << lastVideoSlot_
		     << std::endl;
	    filled = true;
	    lastVideoSlot_ = 0;
	    video_[0] = msg;
	}
	else if (++lastVideoSlot_ < video_.size()) {
	    video_[lastVideoSlot_] = msg;
	}
	else {
	    video_.push_back(msg);
	}
    }

    pruneExtractions();
    pruneRangeTruths();
    pruneBugPlots();
    
    return filled;
}

void
History::Entry::pruneExtractions()
{
    // Prune out extractions that have lived a full life.
    //
    TargetPlotList::iterator pos = extractions_.begin();
    TargetPlotList::iterator end = extractions_.end();
    while (pos != end && pos->getAge() >= extractionsLifeTime_)
	++pos;
    extractions_.erase(extractions_.begin(), pos);
}

void
History::Entry::pruneRangeTruths()
{
    // Prune out range truths that have lived a full life.
    //
    TargetPlotListList::iterator pos = rangeTruths_.begin();
    TargetPlotListList::iterator end = rangeTruths_.end();
    while (pos != end && pos->front().getAge() >= rangeTruthsLifeTime_)
	++pos;
    rangeTruths_.erase(rangeTruths_.begin(), pos);
}

void
History::Entry::pruneBugPlots()
{
    // Prune out user bug plots that have lived a full life.
    //
    TargetPlotList::iterator pos = bugPlots_.begin();
    TargetPlotList::iterator end = bugPlots_.end();
    while (pos != end && pos->getAge() >= bugPlotsLifeTime_)
	++pos;
    bugPlots_.erase(bugPlots_.begin(), pos);
}

void
History::Entry::addBinary(const Messages::BinaryVideo::Ref& msg)
{
    static Logger::ProcLog log("Entry::addBinary", Log());

    if (binary_.empty()) {
	binary_.push_back(msg);
	lastBinarySlot_ = 0;
	return;
    }

    Messages::PRIMessage::Ref last = binary_[lastBinarySlot_];
    if (msg->getShaftEncoding() == last->getShaftEncoding()) {
	binary_[lastBinarySlot_] = msg;
    }
    else {
	if (last->getAzimuthStart() > msg->getAzimuthStart() + M_PI) {
	    LOGDEBUG << "detected full entry - " << lastBinarySlot_
		     << std::endl;
	    lastBinarySlot_ = 0;
	    binary_[0] = msg;
	}
	else if (++lastBinarySlot_ < binary_.size()) {
	    binary_[lastBinarySlot_] = msg;
	}
	else {
	    binary_.push_back(msg);
	}
    }
}

void
History::Entry::addExtractions(const Messages::Extractions::Ref& msg)
{
    QString tag(QString("%1/%2").arg(msg->getMessageSequenceNumber()));
    for (size_t index = 0; index < msg->size(); ++index) {
	extractions_.push_back(TargetPlot(msg[index], tag.arg(index)));
    }
    pruneExtractions();
}

void
History::Entry::addRangeTruth(const Messages::TSPI::Ref& msg)
{
    QString tag = QString::fromStdString(msg->getTag());

    TargetPlotListList::iterator pos = rangeTruths_.begin();
    TargetPlotListList::iterator end = rangeTruths_.end();

    while (pos != end) {
	TargetPlotList& plots(*pos);
	if (plots.front().getTag() == tag) {
	    if (msg->isDropping()) {
		rangeTruths_.erase(pos);
		return;
	    }

	    plots.push_front(TargetPlot(*msg));
	    while (plots.size() > rangeTruthsMaxTrailLength_) {
		plots.pop_back();
	    }

	    return;
	}
	++pos;
    }

    if (! msg->isDropping()) {
	TargetPlotList plots;
	plots.push_back(TargetPlot(*msg));
	rangeTruths_.push_back(plots);
    }

    pruneRangeTruths();
}

void
History::Entry::addBugPlot(const Messages::BugPlot::Ref& msg)
{
    bugPlots_.push_back(TargetPlot(*msg));
    pruneBugPlots();
}

Logger::Log&
History::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.History");
    return log_;
}

History::History(QObject* parent, size_t retentionSize, bool enabled)
    : QObject(parent), retained_(), retentionSize_(retentionSize),
      oldestRetained_(0), liveEntry_(5000, 0), pendingEntry_(0),
      viewedEntry_(&liveEntry_), viewedAge_(0),
      viewedEntryIsRetained_(false), enabled_(enabled)
{
    ;
}

void
History::setRetentionSize(int newSize)
{
    if (enabled_)
	enabled_ = newSize != 0;

    retentionSize_ = newSize;

    // First normalize the buffer so that the oldest entry is first, followed by newer ones.
    //
    size_t size = retained_.size();
    if (size) {
	if (oldestRetained_) {
	    std::rotate(retained_.begin(), retained_.begin() + oldestRetained_,
                        retained_.end());
	    oldestRetained_ = 0;
	}
    }

    // If reducing the number of retained entries, purge the oldest ones from the beginning.
    //
    if (size_t(newSize) < size) {
	size_t reduction = size - newSize;
	for (size_t index = 0; index < reduction; ++index) {
	    if (viewedEntryIsRetained_ &&
                retained_[index] == viewedEntry_) {
		viewedEntryIsRetained_ = false;
	    }
	    else {
		delete retained_[index];
	    }
	}

	retained_.erase(retained_.begin(), retained_.begin() + reduction);

	emit retainedCountChanged(newSize);
    }
}

void
History::addVideo(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("addVideo", Log());
    LOGINFO << std::endl;

    bool filled = liveEntry_.addVideo(msg);

    if (filled) {
	IO::MessageManager::AllocationStats stats =
	    IO::MessageManager::GetAllocationStats();
	LOGDEBUG << "liveEntry pendingEntry - video size: "
		 << liveEntry_.getVideo().size() << " binary size: "
		 << liveEntry_.getBinary().size() << std::endl;
	LOGDEBUG << "msg blocks: " << stats.messageBlocks.numBlocks
		 << ' ' << stats.messageBlocks.numAllocated
		 << ' ' << stats.messageBlocks.numInUse
		 << ' ' << stats.messageBlocks.numFree << std::endl;
    }

    if (enabled_) {

	// If filled up, add the pending entry to the retention queue.
	//
	if (filled && pendingEntry_) {

	    pendingEntry_->freeze();

	    LOGDEBUG << "filled pendingEntry - video size: "
		     << pendingEntry_->getVideo().size() << " binary size: "
		     << pendingEntry_->getBinary().size() << std::endl;

	    // If we have yet to expand to the retention limit, just append.
	    //
	    if (retained_.size() < retentionSize_) {
		LOGDEBUG << "appending to retained array - "
			 << retained_.size() << std::endl;
		oldestRetained_ = 0;
		retained_.push_back(pendingEntry_);
		pendingEntry_ = new Entry(pendingEntry_->getVideo().size(),
                                          pendingEntry_->getBinary().size());
		emit retainedCountChanged(retained_.size());
	    }
	    else {

		LOGDEBUG << "resusing oldest entry - " << oldestRetained_
			 << std::endl;

		// Fetch the oldest and replace with the newest
		//
		Entry* oldest = retained_[oldestRetained_];
		retained_[oldestRetained_++] = pendingEntry_;
		if (oldestRetained_ == retentionSize_)
		    oldestRetained_ = 0;

		// If user is viewing the oldest entry, allocate a new pending entry so that we don't redraw
		// with diffent data.
		//
		if (oldest == viewedEntry_) {
		    LOGDEBUG << "viewing oldest entry" << std::endl;
		    viewedEntryIsRetained_ = false;
		    oldest = new Entry(pendingEntry_->getVideo().size(),
                                       pendingEntry_->getBinary().size());
		}
		else {
		    oldest->clearAll();
		}

		pendingEntry_ = oldest;
	    }

	    if (viewedAge_) {
		++viewedAge_;
		emit currentViewAged(viewedAge_);
	    }
	}

	if (! pendingEntry_) {
	    LOGDEBUG << "creating new pending entry" << std::endl;
	    pendingEntry_ = new Entry(liveEntry_.getVideo().size(),
                                      liveEntry_.getBinary().size());
	}

	pendingEntry_->addVideo(msg);
    }
}

void
History::addBinary(const Messages::BinaryVideo::Ref& msg)
{
    if (liveEntry_.hasVideo()) {
	liveEntry_.addBinary(msg);
	if (pendingEntry_)
	    pendingEntry_->addBinary(msg);
    }
}

void
History::addExtractions(const Messages::Extractions::Ref& msg)
{
    if (liveEntry_.hasVideo()) {
	liveEntry_.addExtractions(msg);
	if (pendingEntry_)
	    pendingEntry_->addExtractions(msg);
    }
}

void
History::addRangeTruth(const Messages::TSPI::Ref& msg)
{
    liveEntry_.addRangeTruth(msg);
    if (pendingEntry_)
	pendingEntry_->addRangeTruth(msg);
}

void
History::addBugPlot(const Messages::BugPlot::Ref& msg)
{
    liveEntry_.addBugPlot(msg);
    if (pendingEntry_)
	pendingEntry_->addBugPlot(msg);
}

void
History::showLiveEntry()
{
    Logger::ProcLog log("showLiveEntry", Log());
    LOGINFO << std::endl;

    if (viewedEntry_ != &liveEntry_) {
	if (! viewedEntryIsRetained_)
	    delete viewedEntry_;
	viewedEntry_ = &liveEntry_;
	viewedAge_ = 0;
	emit currentViewChanged(0);
    }
}

void
History::showEntry(int slot)
{
    static Logger::ProcLog log("showEntry", Log());
    LOGINFO << "slot: " << slot << std::endl;

    if (slot < 0)
	slot = 0;
    else if (size_t(slot) > retained_.size())
	slot = retained_.size();

    LOGDEBUG << "checked slot: " << slot << std::endl;

    if (slot == 0) {
	showLiveEntry();
	return;
    }

    viewedAge_ = slot;

    size_t index = oldestRetained_ + retained_.size() - slot;
    if (index >= retained_.size())
	index -= retained_.size();

    if (index >= retained_.size()) {
	LOGERROR << "invalid slot - " << slot << std::endl;
	return;
    }

    if (viewedEntry_ == retained_[index])
	return;

    if (viewedEntry_ != &liveEntry_ && ! viewedEntryIsRetained_)
	delete viewedEntry_;

    viewedEntry_ = retained_[index];
    viewedEntryIsRetained_ = true;

    emit currentViewChanged(viewedAge_);
}

void
History::clearAll()
{
    liveEntry_.clearAll();
    if (pendingEntry_)
	pendingEntry_->clearAll();
}

void
History::clearVideo()
{
    liveEntry_.clearVideo();
    if (pendingEntry_)
	pendingEntry_->clearVideo();
}

void
History::clearBinary()
{
    liveEntry_.clearBinary();
    if (pendingEntry_)
	pendingEntry_->clearBinary();
}

void
History::clearExtractions()
{
    liveEntry_.clearExtractions();
    if (pendingEntry_)
	pendingEntry_->clearExtractions();
}

void
History::clearRangeTruths()
{
    liveEntry_.clearRangeTruths();
    if (pendingEntry_)
	pendingEntry_->clearRangeTruths();
}

void
History::clearBugPlots()
{
    liveEntry_.clearBugPlots();
    if (pendingEntry_)
	pendingEntry_->clearBugPlots();
}

void
History::setEnabled(bool enabled)
{
    enabled_ = enabled;
    if (! enabled_) {
	showLiveEntry();
	delete pendingEntry_;
	pendingEntry_ = 0;
	for (size_t index = 0; index < retained_.size(); ++index)
	    delete retained_[index];
	retained_.clear();
	oldestRetained_ = 0;
    }
}
