#include <iostream>

#include "DataQueue.h"

using namespace Threading;

DataQueueBase::DataQueueBase() :
    mutex_(Threading::Mutex::Make()), rCond_(Threading::Condition::Make(mutex_)),
    wCond_(Threading::Condition::Make(mutex_))
{
    ;
}

void
DataQueueBase::waitUntilWritable(Signaler& signaler)
{
    while (queueFull()) wCond_->waitForSignal();
    if (queueEmpty()) signaler.arm();
}

void
DataQueueBase::waitUntilReadable(Signaler& signaler)
{
    while (queueEmpty()) rCond_->waitForSignal();
    if (queueFull()) signaler.arm();
}

WorkBufferQueue::WorkBufferQueue(size_t queueSize, size_t blockSize) : raw_(0), free_(queueSize), work_(queueSize)
{
    raw_ = new unsigned char[queueSize * blockSize];
    unsigned char* ptr = raw_;
    while (queueSize--) {
        releaseBuffer(ptr);
        ptr += blockSize;
    }
}

WorkBufferQueue::~WorkBufferQueue()
{
    delete[] raw_;
}
