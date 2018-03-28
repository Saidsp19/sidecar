#include <cassert>

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"

#include "OSCFAR.h"
#include "OSCFAR_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

/** Utility class that maintains an ordered list of sample values from a Video message. When the window slides
    it drops the oldest sample and gains a new sample, while keeping the ordering intact.
*/
class OrderedSlidingWindow {
public:
    OrderedSlidingWindow(Logger::Log& log, size_t windowSize, const Video::Container& initial);

    Video::DatumType getThreshold(size_t index) const { return sorted_[index]; }

    void insertAndRemove(Video::DatumType insert, Video::DatumType remove);

private:
    Video::Container sorted_;
    Logger::Log& log_;
};

OrderedSlidingWindow::OrderedSlidingWindow(Logger::Log& log, size_t windowSize, const Video::Container& initial) :
    sorted_(initial.begin(), initial.begin() + windowSize), log_(log)
{
    // Sort the initial window
    //
    assert(windowSize > 0);
    std::sort(sorted_.begin(), sorted_.end());
}

void
OrderedSlidingWindow::insertAndRemove(Video::DatumType insert, Video::DatumType remove)
{
    if (insert == remove) { return; }

    // Locate the position of the item to remove.
    //
    Video::Container::iterator insertPos;
    Video::Container::iterator removePos;
    if (insert < remove) {
        // Locate the lowest position in the ordered vector where we should find the old item.
        //
        removePos = std::lower_bound(sorted_.begin(), sorted_.end(), remove);

        // Locate the highest position in the ordered vector where we could insert the new item and keep the
        // vector ordered.
        //
        insertPos = std::upper_bound(sorted_.begin(), removePos, insert);

        // If the two positions are the same, just replace.
        //
        if (insertPos == removePos) {
            *insertPos = insert;
        } else {
            // Shift all elements from [insertPos, removePos) up one, thus effectively deleting the entry at
            // removePos, and allowing us to store into insertPos while keeping the elements ordered.
            //
            std::copy_backward(insertPos, removePos, removePos + 1);
            *insertPos = insert;
        }
    } else { // insert > remove

        // Locate the highest position in the ordered vector where we should find the old item.
        //
        removePos = std::upper_bound(sorted_.begin(), sorted_.end(), remove) - 1;

        // Locate the lowest position in the ordered vector where we could insert the new item and keep the
        // vector ordered.
        //
        insertPos = std::lower_bound(removePos, sorted_.end(), insert);

        // If the two positions refer to the same slot, just replace.
        //
        if (insertPos == (removePos + 1)) {
            *removePos = insert;
        } else {
            // Shift all elements from [removePos + 1, insertPos) down one, thus effectively deleting the entry
            // at removePos, and allowing us to store into insertPos while keeping the elements ordered.
            //
            std::copy(removePos + 1, insertPos, removePos);
            --insertPos;
            *insertPos = insert;
        }
    }
}

OSCFAR::OSCFAR(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), windowSize_(Parameter::PositiveIntValue::Make("size",
                                                                              "Number of samples in the<br>"
                                                                              "sliding window",
                                                                              kDefaultSize)),
    thresholdIndex_(Parameter::PositiveIntValue::Make("rank",
                                                      "Percentile <br>"
                                                      "to use for threshold",
                                                      kDefaultRank)),
    alpha_(Parameter::DoubleValue::Make("alpha",
                                        "Multiplier on threshold<br>"
                                        "needed for extraction",
                                        kDefaultAlpha))
{
    reset();
}

bool
OSCFAR::startup()
{
    registerProcessor<OSCFAR, Video>(&OSCFAR::process);
    return registerParameter(windowSize_) && registerParameter(thresholdIndex_) && registerParameter(alpha_) &&
           Algorithm::startup();
}

bool
OSCFAR::process(const Video::Ref& in)
{
    static Logger::ProcLog log("process", getLog());
    double alpha = alpha_->getValue();
    LOGINFO << "pri size: " << in->size() << " alpha " << alpha << std::endl;

    // Nothing to do if the input data size is smaller than our window size.
    //
    size_t windowSize = windowSize_->getValue();
    if (windowSize > in->size()) {
        LOGERROR << "windowSize > in->size()" << std::endl;
        return false;
    }

    // Verify that the threshold index we use is valid in the sliding window.
    //
    size_t percentile = thresholdIndex_->getValue();
    double indexIntoWindow = (percentile / 100.0) * windowSize;
    size_t thresholdIndex = static_cast<size_t>(std::floor(indexIntoWindow));

    LOGINFO << "thresholdIndex " << thresholdIndex << std::endl;
    if (thresholdIndex >= windowSize) {
        getController().setError("Threshold index >= window size");
        return false;
    }

    size_t halfWindowSize = windowSize / 2;

    // Create our sliding window. It should remain sorted as we slide over the input data.
    //
    OrderedSlidingWindow slidingWindow(getLog(), windowSize, in->getData());

    // Calculate the number of times we will apply the window. This unsigned arithmetic is safe to do because
    // above we guarantee that windowSize_ <= in->size().
    //
    size_t limit = in->size() - windowSize;

    BinaryVideo::Ref out(BinaryVideo::Make(getName(), in));
    out->resize(in->size(), 0);

    // This was written as a for loop, but that is incorrect because in the last iteration the call to
    // OrderedSlidingWindow::insertAndRemove() got called with an invalid index into the input message. Instead,
    // do the termination check from inside the loop, before the call to insertAndRemove().
    //
    size_t index = 0;
    while (1) {
        size_t pos = index + halfWindowSize;
        bool passed = in[pos] > (alpha * slidingWindow.getThreshold(thresholdIndex));
        out[pos] = passed;

        // Check that we can safely continue and index into the input array.
        //
        if (index == limit) { break; }

        // Remove the element that will no longer be valid when the sliding window moves to the next sample, and
        // add the new element that is now visible in the window.
        //
        slidingWindow.insertAndRemove(in[index + windowSize], in[index]);
        ++index;
    }

    // Finally emit the output message.
    //
    bool rc = send(out);
    LOGDEBUG << "send: " << rc << std::endl;
    return rc;
}

void
OSCFAR::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kWindowSize, windowSize_->getValue());
    status.setSlot(kThresholdIndex, thresholdIndex_->getValue());
    status.setSlot(kAlpha, alpha_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    return Algorithm::FormatInfoValue(QString("WindowSize: %1  ThresholdPercentile: %2  Alpha: %3")
                                          .arg(static_cast<int>(status[OSCFAR::kWindowSize]))
                                          .arg(static_cast<int>(status[OSCFAR::kThresholdIndex]))
                                          .arg(static_cast<double>(status[OSCFAR::kAlpha])));
}

extern "C" ACE_Svc_Export Algorithm*
OSCFARMake(Controller& controller, Logger::Log& log)
{
    return new OSCFAR(controller, log);
}
