#include "RingBuffer.h"

using namespace Utils;

RingBufferBase::RingBufferBase(int capacity)
    : writeOffset_(0), readOffset_(0), capacity_(capacity), size_(0)
{
    ;
}

int
RingBufferBase::getReadOffset()
{
    auto ro = readOffset_;
    readOffset_ = (ro + 1) % capacity_;
    return ro;
}

int
RingBufferBase::getWriteOffset()
{
    auto wo = writeOffset_;
    writeOffset_ = (wo + 1) % capacity_;
    return wo;
}
