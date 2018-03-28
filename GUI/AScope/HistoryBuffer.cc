#include "Time/TimeStamp.h"

#include "HistoryBuffer.h"

using namespace SideCar::Messages;
using namespace SideCar::GUI::AScope;

HistoryBuffer::HistoryBuffer(int duration) : buffers_(), frame_(new HistoryFrame), duration_(duration)
{
    setDuration(duration);
}

void
HistoryBuffer::setDuration(int duration)
{
    duration_ = duration;
}

void
HistoryBuffer::record(int bufferIndex, const PRIMessage::Ref& msg)
{
    Time::TimeStamp limit = msg->getCreatedTimeStamp();
    limit -= duration_;

    while (bufferIndex >= buffers_.size()) { buffers_.append(new MessageBuffer); }

    MessageBuffer* buffer = buffers_[bufferIndex];
    buffer->append(msg);

    size_ = 0;
    for (int index = 0; index < buffers_.size(); ++index) {
        buffer = buffers_[index];
        MessageBuffer::iterator pos = buffer->begin();
        while (pos != buffer->end() && (*pos)->getCreatedTimeStamp() < limit) { ++pos; }

        if (pos != buffer->begin()) { buffer->erase(buffer->begin(), pos); }

        if (size_ < buffer->size()) size_ = buffer->size();
    }
}

const HistoryFrame&
HistoryBuffer::getFrame(int index) const
{
    frame_->clear();
    for (int index = 0; index < buffers_.size(); ++index) {
        MessageBuffer* buffer = buffers_[index];
        if (index < buffer->size())
            frame_->add((*buffer)[index]);
        else
            frame_->add(PRIMessage::Ref());
    }
    return *frame_;
}

void
HistoryBuffer::addMessageBuffer()
{
    buffers_.append(new MessageBuffer);
}

void
HistoryBuffer::removeMessageBuffer(int index)
{
    delete buffers_.takeAt(index);
    size_ = 0;
    for (int index = 0; index < buffers_.size(); ++index) {
        MessageBuffer* buffer = buffers_[index];
        if (buffer->size() > size_) size_ = buffer->size();
    }
}

void
HistoryBuffer::clear()
{
    for (int index = 0; index < buffers_.size(); ++index) { buffers_[index]->clear(); }
}
