// -*- C++ -*-

#include <algorithm>

#include "Utils/Utils.h"	// for LCM
#include "Utils/Pool.h"

using namespace Utils;

bool Pool::validateRelease_ = false;
bool Pool::objectSizeWarning_ = false;

bool
Pool::SetValidateRelease(bool flag)
{
    auto prev = validateRelease_;
    validateRelease_ = flag;
    return prev;
}

bool
Pool::SetObjectSizeWarning(bool flag)
{
    auto prev = objectSizeWarning_;
    objectSizeWarning_ = flag;
    return prev;
}

Pool::Pool(std::size_t objectSize, std::size_t objectsPerBlock) throw(std::invalid_argument)
    : free_(0), blocks_(), objectSize_(objectSize), chunkSize_(Utils::LCM(objectSize, sizeof(FreeStore))),
      chunksPerBlock_(objectsPerBlock), blockSize_(chunkSize_ * objectsPerBlock), numInUse_(0)
{
    if (objectSize < 1) throw std::invalid_argument("objectSize");
    if (objectsPerBlock < 1) throw std::invalid_argument("objectsPerBlock");
}

Pool::~Pool() throw()
{
    std::for_each(blocks_.begin(), blocks_.end(), [](auto b){delete [] b;});
}

void*
Pool::allocate(std::size_t size) throw(std::bad_alloc)
{
    if (size != objectSize_) {
	if (objectSizeWarning_) {
	    std::clog << "Pool::allocate: size asked for does not match pool " << "size - " << size << ' '
                      << objectSize_ << '\n';
        }
	return ::operator new(size);
    }

    if (! free_) newBlock();

    // Unlink chunk from free list and return.
    //
    auto ptr = free_;
    free_ = free_->next;
    ++numInUse_;
    return ptr;
}

void
Pool::release(void* ptr, std::size_t size) throw()
{
    if (! ptr) return;
    if (size != objectSize_) {
	if (objectSizeWarning_)
	    std::clog << "Pool::release: size of object does not match pool " << "size - " << size << ' '
                      << objectSize_ << '\n';
	::operator delete(ptr);
	return;
    }

    // If configured to do so, Check that pointer was actually given out by the allocate method.
    //
    if (validateRelease_ && ! isPoolObject(ptr)) abort();

    // Add chunk to free list.
    //
    auto obj = static_cast<FreeStore*>(ptr);
    obj->next = free_;
    free_ = obj;
    --numInUse_;
}

void
Pool::newBlock() throw(std::bad_alloc)
{
    // Allocate new block to hold N number of objects and add to list of allocated blocks.
    //
    char* raw = new char[blockSize_];
    blocks_.push_back(raw);

    // Point to first chunk in the allocated block, and add the objects to the free store.
    //
    for (size_t count = 0; count < chunksPerBlock_; ++count) {
	reinterpret_cast<FreeStore*>(raw)->next = free_;
	free_ = reinterpret_cast<FreeStore*>(raw);
	raw += chunkSize_;
    }
}

bool
Pool::isPoolObject(void* ptr) const
{
    return std::find_if(blocks_.begin(), blocks_.end(),
                        [ptr, blockSize = blockSize_](auto b) {
                            return ptr >= b && ptr < (b + blockSize);
                        }) != blocks_.end();
}

Pool::AllocationStats
Pool::getAllocationStats() const
{
    AllocationStats stats;
    stats.numBlocks = numBlocks();
    stats.numAllocated = numAllocated();
    stats.numInUse = numInUse();
    stats.numFree = numFree();
    return stats;
}
