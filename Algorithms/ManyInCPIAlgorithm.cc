#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/ChannelBuffer.h"
#include "Logger/Log.h"

#include "ManyInCPIAlgorithm.h"

#include "QtCore/QVariant"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
ManyInCPIAlgorithm::ManyInCPIAlgorithm(Controller& controller, Logger::Log& log, bool enabled, size_t maxBuffersize,
                                       size_t cpiSpan) :
    Super(controller, log, enabled, maxBuffersize),
    cpiSpan_(Parameter::PositiveIntValue::Make("cpiSpan", "Number of PRIs in a CPI", cpiSpan))
{
    ;
}

// Startup routine. as shown below.
//
bool
ManyInCPIAlgorithm::startup()
{
    return Super::startup() && registerParameter(cpiSpan_);
}

/** Functor used by ManyInCPIAlgorithm::pruneToCPIBoundary() that prunes ChannelBuffer of messages with an
    expected CPI span value given to the constructor.
*/
struct ChannelCPIPrune {
    ChannelCPIPrune(size_t cpiSpan) : cpiSpan_(cpiSpan), ok_(true), valid_(false) {}

    void operator()(ChannelBuffer* channel)
    {
        if (!channel->isEnabled()) return;

        valid_ = true;

        // Iterate through the buffer looking for the CPI boundary which is currently identified by checking
        // the prfEncoding field in the PRI Message header. Currently, we look for M_(i).code !=
        // M2_(i+1).code. If i != cpiSpan-1, then we have an invalid CPI (ie, it's missing messages).
        //
        ChannelBuffer::const_iterator itr = channel->begin();
        Messages::PRIMessage::Ref last = channel->getFront();
        size_t pos = 0; // location of start of next CPI
        bool cpiBoundary = false;
        ++itr; // move to next PRI message
        ++pos;
        for (; itr != channel->end() && pos <= cpiSpan_; ++itr, ++pos) {
            // Check for CPI boundary
            //
            cpiBoundary = (last->getRIUInfo().prfEncoding != (*itr)->getRIUInfo().prfEncoding);

            if (cpiBoundary) {
                // Found the boundary, ensure it is located at the proper index which is equal to the
                // cpiSpan. This will implicitly check that no messages were dropped between the first PRI
                // message of a CPI and the last one.
                //
                if (pos != cpiSpan_) {
                    // Need to prune off incomplete CPI by pruning to the sequence number of the start of
                    // the next CPI
                    //
                    if (!channel->pruneToSequenceCounter((*itr)->getSequenceCounter())) ok_ = false;

                } // end if (pos != cpiSpan_)
            }     // end if (delta > thresh)
        }         // end for loop
    }             // end void operator()

    operator bool() const { return valid_ && ok_; }

    size_t cpiSpan_;
    bool ok_;
    bool valid_;
};

/** Functor used by ManyInCPIAlgorithm::verifyChannelSize() to ensure that we have at least a complete CPI in
    the channel buffer.
*/
struct VerifyChannelSize {
    VerifyChannelSize(size_t cpiSpan) : cpiSpan_(cpiSpan), ok_(true), valid_(false) {}

    void operator()(ChannelBuffer* channel)
    {
        if (channel->isEnabled()) valid_ = true;
        if (channel->size() <= cpiSpan_) ok_ = false;
    }

    operator bool() { return valid_ && ok_; }

    size_t cpiSpan_;
    bool ok_;
    bool valid_;
};

bool
ManyInCPIAlgorithm::verifyChannelSize(size_t min)
{
    return std::for_each(channels_.begin(), channels_.end(), VerifyChannelSize(min));
}

bool
ManyInCPIAlgorithm::pruneToCPIBoundary(size_t cpiSpan)
{
    // Prune all of the input channels. Return true iff there is a complete CPI message in all of the channels
    // for us to use.
    //
    return std::for_each(channels_.begin(), channels_.end(), ChannelCPIPrune(cpiSpan));
}

bool
ManyInCPIAlgorithm::processChannels()
{
    return processCPI();
}

bool
ManyInCPIAlgorithm::processMessageReceived()
{
    // NOTE: we should only get called from ChannelBuffer::addData(). Therefore, we can safely assume that there
    // is at least one message in one of our channels.
    //
    uint32_t maxSeq = getMaxSequenceCounter();

    size_t cpiSpan = cpiSpan_->getValue();

    // Determine if all of our channels have a message with the same sequence number.
    //
    bool ok = pruneToSequenceCounter(maxSeq) && pruneToCPIBoundary(cpiSpan) && verifyChannelSize(cpiSpan);

    if (!ok) return true;

    // We have a message in every channel to process. Derived classes must have an implementation of
    // processChannels().
    //
    return processChannels();
}
