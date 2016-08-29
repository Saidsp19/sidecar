#include <algorithm>		// for std::transform
#include <functional>		// for std::plus
#include <math.h>

#include "boost/bind.hpp"

#include "IO/MessageManager.h"
#include "Logger/Log.h"

#include "MofN.h"
#include "MofN_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

MofN::MofN(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      numPRIs_(Parameter::PositiveIntValue::Make("numPRIs", "Num PRIs", kDefaultNumPRIs)),
      numGates_(Parameter::PositiveIntValue::Make("numGates", "Num Gates", kDefaultNumGates)),
      threshold_(Threshold::Make("threshold", "Score Threshold", kDefaultThreshold))
{
    numPRIs_->connectChangedSignalTo(boost::bind(&MofN::numPRIsChanged, this, _1));
    numGates_->connectChangedSignalTo(boost::bind(&MofN::numGatesChanged, this, _1));
    threshold_->connectChangedSignalTo(boost::bind(&MofN::thresholdChanged, this, _1));
}

bool
MofN::startup()
{
    calculateThresholdValue();
    registerProcessor<MofN,Messages::BinaryVideo>(&MofN::process);
    return registerParameter(enabled_) &&
	registerParameter(numPRIs_) &&
	registerParameter(numGates_) &&
	registerParameter(threshold_) &&
	Algorithm::startup();
}

bool
MofN::reset()
{
    retained_.clear();
    for (int index = 0; index < numPRIs_->getValue() + 1; ++index) {
	retained_.push_back(RetainedEntry());
    }
    retainedCount_ = 0;
    oldestIndex_ = 0;
    newestIndex_ = 0;
    runningCounts_.clear();
    return true;
}

struct Updater
{
    uint16_t threshold_;
    MofN::DetectionCountVector::const_iterator del_;
    MofN::DetectionCountVector::const_iterator add_;
    Messages::BinaryVideo::Container& out_;
    Updater(uint16_t threshold, MofN::DetectionCountVector::const_iterator del,
            MofN::DetectionCountVector::const_iterator add, Messages::BinaryVideo::Container& out)
	: threshold_(threshold), del_(del), add_(add), out_(out) {}

    MofN::DetectionCountType operator()(MofN::DetectionCountType v)
	{
	    v += (*add_++ - *del_++);
	    out_.push_back(v >= threshold_);
	    return v;
	}
};

bool
MofN::process(const Messages::BinaryVideo::Ref& video)
{
    static Logger::ProcLog log("process", getLog());
    LOGDEBUG << std::endl;

    if (! enabled_->getValue()) {
	return send(video);
    }

    // Detect mismatch between current runningCounts_.size() and sample count in the message. Grow
    // runningCounts_.size() if necessary, but leave the message alone if it is smaller.
    //
    size_t priLen = video->size();
    if (runningCounts_.size() < priLen) {
	runningCounts_.resize(priLen, 0);
    }
    else if (runningCounts_.size() > priLen) {
	LOGWARNING << "incoming message too small - expected: " << runningCounts_.size() << " got: " << priLen
                   << std::endl;
    }

    // We save the result of counting in the range dimension so that we won't have to repeat the calculation for
    // subsequent PRIs. We will ultimately keep around vectors for the previous numPRIs PRIs
    //
    size_t numPRIs = numPRIs_->getValue();
    retained_[newestIndex_].video = video;

    // Use the oldest + 1 slot to hold our sample and detection counts. We need the oldest detection counts below to
    // keep our running count up-to-date so we cannot use that slot yet.
    //
    DetectionCountVector& detections(retained_[newestIndex_++].detectionCounts);
    if (newestIndex_ == retained_.size()) {
	newestIndex_ = 0;
    }

    detections.resize(runningCounts_.size(), 0);

    // The first stage in the calculation is to count the number of detections within a "numGates" sized window window
    // around each range cell.
    //
    Messages::BinaryVideo::const_iterator videoIter(video->begin());
    size_t numGates = numGates_->getValue();
    size_t halfWindow = numGates / 2;
    size_t gateIndex = 0;

    // Calculate partial windows at the start of the PRI. Update the running counter, but don't record anything get.
    //
    DetectionCountType count = 0;
    while (gateIndex++ < halfWindow) {
	count += (*videoIter++ ? 1 : 0);
    }

    // Keep updating the counter, but now we can store the counter results.
    //
    DetectionCountVector::iterator detectionsIter(detections.begin());
    while (gateIndex++ < numGates) {
	count += (*videoIter++ ? 1 : 0);
	*detectionsIter++ = count;
    }

    // Even better -- we can store and remove the oldest sample.
    //
    Messages::BinaryVideo::const_iterator oldestIter(video->begin());
    while (gateIndex++ < priLen) {
	count += (*videoIter++ ? 1 : 0);
	*detectionsIter++ = count;
	count -= (*oldestIter++ ? 1 : 0);
    }

    // Reached the end of the PRI message. Go back and finish calculating windowed counts. Be careful not to move
    // beyond the sample data which could happen if priLen < runningCounts_.size()
    //
    Messages::BinaryVideo::const_iterator videoEnd(video->end());
    while (oldestIter < videoEnd) {
	*detectionsIter++ = count;
	count -= (*oldestIter++ ? 1 : 0);
    }

    // Finally, handle the case where priLen < runningCounts_.size(). Just set to zero.
    //
    DetectionCountVector::iterator detectionsEnd(detections.end());
    while (detectionsIter < detectionsEnd) {
	*detectionsIter++ = 0;
    }

    // Update the running counts vector with the detections we just calculated. If we don't have a desired number of
    // PRI messages, then just add the calculated counts for each gate to the running count tally.
    //
    if (retainedCount_ < numPRIs) {
	++retainedCount_;
	std::transform(runningCounts_.begin(), runningCounts_.end(), detections.begin(), runningCounts_.begin(),
                       std::plus<DetectionCountType>());
	return true;
    }

    // Create a new BinaryVideo message with gate values that are true/false depending on whether the MofN window
    // surrounding the gate has enough ON values to pass a threshold value. Base our outgoing message on the middle
    // message in our MofN window.
    //
    size_t index = (oldestIndex_ + numPRIs / 2);
    if (index >= retained_.size()) {
	index -= retained_.size();
    }

    Messages::BinaryVideo::Ref midPoint((retained_.begin() + index)->video);
    Messages::BinaryVideo::Ref out(Messages::BinaryVideo::Make(getName(), midPoint));
    out->reserve(runningCounts_.size());

    // Add the new counts vector to and subtract the oldest counts vector from the running count. A side-effect of this
    // is that the output message gets the boolean values based on whether the updated sample count values match or
    // pass thresholdValue_.
    //
    std::transform(runningCounts_.begin(), runningCounts_.end(), runningCounts_.begin(),
                   Updater(thresholdValue_, retained_[oldestIndex_].detectionCounts.begin(), detections.begin(),
                           out->getData()));
    ++oldestIndex_;
    if (oldestIndex_ == retained_.size()) {
	oldestIndex_ = 0;
    }

    LOGDEBUG << *out.get() << std::endl;
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

void
MofN::calculateThresholdValue()
{
    thresholdValue_ = static_cast<uint16_t>(
        ::round(threshold_->getValue() * numPRIs_->getValue() * numGates_->getValue()));
}

void
MofN::numPRIsChanged(const Parameter::PositiveIntValue& value)
{
    static Logger::ProcLog log("numPRIsChanged", getLog());
    LOGINFO << value << std::endl;
    reset();
    calculateThresholdValue();
}

void
MofN::numGatesChanged(const Parameter::PositiveIntValue& value)
{
    static Logger::ProcLog log("numGatesChanged", getLog());
    LOGINFO << value << std::endl;
    reset();
    calculateThresholdValue();
}

void
MofN::thresholdChanged(const Threshold& value)
{
    static Logger::ProcLog log("thresholdChanged", getLog());
    LOGINFO << value << std::endl;
    calculateThresholdValue();
}

void
MofN::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kNumPRIs, numPRIs_->getValue());
    status.setSlot(kNumGates, numGates_->getValue());
    status.setSlot(kThreshold, threshold_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    int numPRIs = status[MofN::kNumPRIs];
    int numGates = status[MofN::kNumGates];
    double threshold = status[MofN::kThreshold];
    return Algorithm::FormatInfoValue(QString("Num PRIs: %1  Num Gates: %2  Threshold: %3")
                                      .arg(numPRIs).arg(numGates).arg(threshold));
}

extern "C" ACE_Svc_Export Algorithm*
MofNMake(Controller& controller, Logger::Log& log)
{
    return new MofN(controller, log);
}
