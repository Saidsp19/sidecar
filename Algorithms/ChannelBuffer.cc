#include <sstream>

#include "ChannelBuffer.h"
#include "ManyInAlgorithm.h"

using namespace SideCar::Algorithms;

std::string
ChannelBuffer::MakeGenericShortName(int index)
{
    std::ostringstream os;
    os << "input" << (index + 1) << "Enabled";
    return os.str();
}

std::string
ChannelBuffer::MakeGenericLongName(int index)
{
    std::ostringstream os;
    os << "Input Channel " << (index + 1) << " Enabled";
    return os.str();
}

ChannelBuffer::ChannelBuffer(ManyInAlgorithm& processor, size_t channelIndex, size_t maxBufferSize) :
    processor_(processor), enabledParam_(), channelIndex_(channelIndex), maxBufferSize_(maxBufferSize),
    nextSequenceCounter_(0), container_(), enabled_(true)
{
    ;
}

ChannelBuffer::~ChannelBuffer()
{
    ;
}

bool
ChannelBuffer::makeEnabledParameter()
{
    return makeEnabledParameter(MakeGenericShortName(channelIndex_), MakeGenericLongName(channelIndex_));
}

bool
ChannelBuffer::makeEnabledParameter(const std::string& shortName, const std::string& longName)
{
    enabledParam_ = Parameter::BoolValue::Make(shortName, longName, enabled_);
    enabledParam_->connectChangedSignalTo([this](auto& v) { enabledChanged(v); });
    return processor_.registerParameter(enabledParam_);
}

void
ChannelBuffer::enabledChanged(const Parameter::BoolValue& parameter)
{
    if (enabled_ != parameter.getValue()) { setEnabled(parameter.getValue()); }
}

bool
ChannelBuffer::pruneToSequenceCounter(uint32_t sequenceCounter)
{
    static const uint32_t kWrapped = 1 << 16;

    // Skip entries until we have a message with a sequenceCounter that is not older than the given one.
    //
    while (!isEmpty() &&
           ((sequenceCounter > nextSequenceCounter_) ||
            (sequenceCounter < nextSequenceCounter_ && nextSequenceCounter_ - sequenceCounter > kWrapped))) {
        popFrontInternal();
    }
    return !isEmpty() && nextSequenceCounter_ == sequenceCounter;
}

void
ChannelBuffer::setMaxBufferSize(size_t maxBufferSize)
{
    maxBufferSize_ = maxBufferSize;
    pruneToMaxBufferSize();
}

void
ChannelBuffer::setEnabled(bool value)
{
    enabled_ = value;
    reset();
    processor_.channelEnabledChanged(this);
}

void
ChannelBuffer::reset()
{
    nextSequenceCounter_ = 0;
    container_.clear();
}

bool
ChannelBuffer::addData(const Messages::PRIMessage::Ref& msg)
{
    if (!isEnabled()) return true;

    if (container_.empty()) nextSequenceCounter_ = msg->getSequenceCounter();
    container_.push_back(msg);
    pruneToMaxBufferSize();

    return processor_.processMessageReceived();
}

void
ChannelBuffer::popFrontInternal()
{
    if (!isEmpty()) {
        container_.pop_front();
        nextSequenceCounter_ = container_.empty() ? 0 : container_.front()->getSequenceCounter();
    }
}

void
ChannelBuffer::pruneToMaxBufferSize()
{
    while (size() > maxBufferSize_) popFrontInternal();
}
