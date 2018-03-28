#include "Logger/Log.h"
#include "XMLRPC/XmlRpcValue.h"

#include "ChannelBuffer.h"
#include "Controller.h"
#include "ManyInAlgorithm.h"

using namespace SideCar::Algorithms;

QString
ManyInAlgorithm::GetFormattedStats(const IO::StatusBase& status)
{
    if (!status[kEnabled]) return "Disabled  ";

    const XmlRpc::XmlRpcValue& v(status[kChannelStats]);
    QString output;
    for (int index = 0; index < v.size();) {
        int id = v[index++];
        int size = v[index++];
        if (size) output += QString("C%1[%2]  ").arg(id).arg(size);
    }

    return output;
}

ManyInAlgorithm::ManyInAlgorithm(Controller& controller, Logger::Log& log, bool enabled, size_t maxBufferSize) :
    Algorithm(controller, log), channels_(), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", enabled)),
    maxBufferSize_(Parameter::PositiveIntValue::Make("maxBufferSize", "Max channel buffer size", maxBufferSize))
{
    maxBufferSize_->connectChangedSignalTo([this](auto& v) { maxBufferSizeChanged(v); });
}

bool
ManyInAlgorithm::startup()
{
    Logger::ProcLog log("startup", getLog());
    int numChannels = getController().getNumInputChannels();
    LOGINFO << "numChannels: " << numChannels << std::endl;

    // Create a ChannelBuffer for each configured input channel.
    //
    int maxBufferSize = maxBufferSize_->getValue();
    for (int index = 0; index < numChannels; ++index) {
        const std::string& name(getController().getInputChannel(index).getName());
        ChannelBuffer* channel = makeChannelBuffer(index, name, maxBufferSize);
        if (!channel) return false;
        channels_.push_back(channel);
    }

    // Register our runtime parameters and let our parent class startup.
    //
    return Super::startup() && registerParameter(enabled_) && registerParameter(maxBufferSize_);
}

bool
ManyInAlgorithm::reset()
{
    for (auto c : channels_) c->reset();
    return Super::reset();
}

bool
ManyInAlgorithm::shutdown()
{
    // !!! Do not delete objects in channels_ since they are also registered processors, which get deleted in
    // Algorithm::~Algorithm().
    //
    channels_.clear();
    return Super::shutdown();
}

/** Functor used by ManyInAlgorithm::getMaxSequenceCounter() to locate the oldest sequence counter common to all
    channels. Remembers the largest sequence counter seen when calling ChannelBuffer::getNextSequenceCounter().
    Since sequence counters are monotonically increasing (excepting for wrap-around) the largest sequence number
    seen is the only possible candidate that could be common across all of them.
*/
struct ChannelMaxSequence {
    ChannelMaxSequence() : maxSeq_(0) {}

    void operator()(ChannelBuffer* channel)
    {
        static const uint32_t kWrapped = (1 << 16);
        if (channel->isEmpty() || !channel->isEnabled()) return;
        uint32_t seq = channel->getNextSequenceCounter();

        // Check for and handle wrap-around of sequence numbers.
        //
        if ((seq < maxSeq_ && maxSeq_ - seq > kWrapped) || (seq > maxSeq_)) { maxSeq_ = seq; }
    }

    operator uint32_t() const { return maxSeq_; }

    uint32_t maxSeq_;
};

uint32_t
ManyInAlgorithm::getMaxSequenceCounter() const
{
    return std::for_each(channels_.begin(), channels_.end(), ChannelMaxSequence());
}

/** Functor used by ManyInAlgorithm::pruneToSequenceCounter() that prunes ChannelBuffer of messages with an
    sequence counter older than that given to the constructor.
*/
struct ChannelPrune {
    ChannelPrune(uint32_t maxSeq) : maxSeq_(maxSeq), ok_(true), valid_(false) {}

    void operator()(ChannelBuffer* channel)
    {
        if (channel->isEnabled()) {
            valid_ = true;

            // ChannelBuffer::pruneToSequenceCounter() returns true if there is still a message in the
            // buffer after the pruning.
            //
            if (!channel->pruneToSequenceCounter(maxSeq_)) ok_ = false;
        }
    }

    operator bool() const { return valid_ && ok_; }

    uint32_t maxSeq_;
    bool ok_;
    bool valid_;
};

bool
ManyInAlgorithm::pruneToSequenceCounter(uint32_t sequenceCounter)
{
    // Prune all of the input channels. Return true iff there is a message in all of the channels for us to use.
    //
    return std::for_each(channels_.begin(), channels_.end(), ChannelPrune(sequenceCounter));
}

void
ManyInAlgorithm::setInfoSlots(IO::StatusBase& status)
{
    // !!! NOTE: this routine runs in a thread different from the one that runs the algorithm, so BE CAREFUL
    // !!! with what and how you access state data.
    //
    status.setSlot(kEnabled, enabled_->getValue());

    // The value channels_.size() does not change during the lifetime of the algorithm -- it is set by the XML
    // configuration for the algorithm, so we are safe to access the channels_ vector at any time from any
    // thread.
    //
    XmlRpc::XmlRpcValue::ValueArray* v = new XmlRpc::XmlRpcValue::ValueArray;

    for (auto channel : channels_) {
        // Race condition here: channel->isEnabled() may become false while we fetched the following values,
        // however the values we fetch don't depend on its value so I think we are safe.
        //
        if (channel->isEnabled()) {
            v->push_back(int(channel->getChannelIndex()));
            v->push_back(int(channel->size()));
        }
    }

    // Set the number of enabled channels we found above.
    //
    status.setSlot(kChannelStats, v);
}

void
ManyInAlgorithm::channelEnabledChanged(ChannelBuffer* channel)
{
    // Derived classes may override to handle enabled state changes for a ChannelBuffer.
    //
}

bool
ManyInAlgorithm::processMessageReceived()
{
    // NOTE: we should only get called from ChannelBuffer::addData(). Therefore, we can safely assume that there
    // is at least one message in one of our channels.
    //
    uint32_t maxSeq = getMaxSequenceCounter();

    // Determine if all of our channels have a message with the same sequence number.
    //
    if (!pruneToSequenceCounter(maxSeq)) return true;

    // We have a message in every channel to process. Derived classes must have an implementation of
    // processChannels().
    //
    return processChannels();
}

void
ManyInAlgorithm::maxBufferSizeChanged(const Parameter::PositiveIntValue& param)
{
    // Update all of the input channels to hold the new maximum value of messages.
    //
    size_t value = param.getValue();
    for (auto channel : channels_) channel->setMaxBufferSize(value);
}

ChannelBuffer*
ManyInAlgorithm::findChannelBuffer(const std::string& name) const
{
    size_t index = getController().getInputChannelIndex(name);
    return index < getController().getNumInputChannels() ? channels_[index] : nullptr;
}
