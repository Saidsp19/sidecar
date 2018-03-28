#include "Logger/Log.h"

#include "Channel.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::DownConverterUtils;

Logger::Log&
Channel::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Algorithms.DownConverter.Channel");
    return log_;
}

Channel::Channel(DownConverter& master, size_t maxBufferSize) :
    Super(this, &Channel::addData), master_(master), buffer_(), maxBufferSize_(maxBufferSize)
{
    ;
}

void
Channel::setMaxBufferSize(size_t maxBufferSize)
{
    static Logger::ProcLog log("setMaxBufferSize", Log());
    LOGINFO << "maxBufferSize: " << maxBufferSize_ << std::endl;

    maxBufferSize_ = maxBufferSize;
    while (buffer_.size() > maxBufferSize) buffer_.pop_front();
    updateNextSequence();

    LOGDEBUG << "buffer_.size():: " << buffer_.size() << std::endl;
}

bool
Channel::addData(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("addData", Log());
    LOGINFO << std::endl;

    buffer_.push_back(msg);

    // Remove the oldest messages until our queue size is acceptable.
    //
    while (buffer_.size() > maxBufferSize_) buffer_.pop_front();
    updateNextSequence();

    LOGDEBUG << "buffer_.size():: " << buffer_.size() << std::endl;
    return master_.process();
}

void
Channel::getSlice(int offset, int span, VsipComplexVector& slice)
{
    static Logger::ProcLog log("getSlice", Log());
    LOGINFO << "offset: " << offset << " span: " << span << std::endl;

    if (buffer_.empty()) {
        LOGERROR << "called with empty buffer!" << std::endl;
        return;
    }

    // Pop the message from the buffer stack. We hold a reference to the message so it is still safe to use.
    //
    Messages::Video::Ref msg(popData());
    Messages::Video::Container& container(msg->getData());

    // Restrict the copying below to valid gates. May resize span to be smaller than the given value.
    //
    int limit = span;
    if ((offset + span) * 2 > int(container.size())) {
        limit = container.size() / 2 - offset;
        LOGDEBUG << "new limit: " << limit << std::endl;
        if (limit <= 0) {
            LOGERROR << "nothing to copy - offset: " << offset << " limit: " << limit << " size: " << container.size()
                     << std::endl;
            return;
        }
    }

    Messages::Video::DatumType* ptr = &container[offset * 2];
    for (int index = 0; index < limit; ++index, ptr += 2) { slice.put(index, ComplexType(*ptr, *(ptr + 1))); }

    LOGDEBUG << "limit: " << limit << " span: " << span << std::endl;
    while (limit < span) slice.put(limit++, ComplexType());
}

bool
Channel::prune(uint32_t sequenceCounter)
{
    static Logger::ProcLog log("prune", Log());
    LOGINFO << "sequenceCounter: " << sequenceCounter << std::endl;

    static const uint32_t kWrapAround = (1 << 16);
    while (!buffer_.empty() && ((sequenceCounter > nextSequence_) ||
                                (sequenceCounter < nextSequence_ && nextSequence_ - sequenceCounter > kWrapAround))) {
        buffer_.pop_front();
        updateNextSequence();
    }

    LOGDEBUG << "buffer_.size(): " << buffer_.size() << std::endl;
    return buffer_.empty();
}

Messages::Video::Ref
Channel::popData()
{
    Messages::Video::Ref msg(buffer_.front());
    buffer_.pop_front();
    updateNextSequence();
    return msg;
}

void
Channel::updateNextSequence()
{
    if (buffer_.empty())
        nextSequence_ = 0;
    else
        nextSequence_ = buffer_.front()->getRIUInfo().sequenceCounter;
}
