#ifndef UTILS_RINGBUFFER_H // -*- C++ -*-
#define UTILS_RINGBUFFER_H

#include <memory>
#include <vector>

namespace Utils {

/** Common base class for ring buffers. Handles all book-keeping.
 */
class RingBufferBase
{
public:
    
    /** Constructor.

        \param capacity maximum number of objects the buffer can hold
    */
    RingBufferBase(int capacity);

    /** Obtain the capacity of the buffer

        \return capacity
    */
    int getCapacity() const { return capacity_; }

    /** Obtain the number of objects held by the buffer.

        \return size
    */
    int getSize() const { return size_; }

    /** Determine if the buffer is empty.

        \return true if so
    */
    bool isEmpty() const { return getSize() == 0; }

    /** Determine if the buffer is full

        \return true if so
    */
    bool isFull() const { return getSize() == getCapacity(); }

protected:

    /** Obtain the current read offset, then increment the offset before returning.

        \return read offset
    */
    int getReadOffset();

    /** Obtain the current write offset, then increment the offset before returning.

        \return write offset
    */
    int getWriteOffset();

    /** Increment the number of objects in the buffer
     */
    void added() { ++size_; }

    /** Decrement the number of objects in the buffer.
     */
    void removed() { --size_; }

private:
    volatile int writeOffset_;
    volatile int readOffset_;
    int capacity_;
    std::atomic<int> size_;
};

/** Template class that defines a type-safe ring buffer. Provides methods to add to and take from the buffer.
    Uses std::vector for the container that holds the objects.
*/
template <typename T>
class TRingBuffer : public RingBufferBase
{
public:

    /** Constructor.

        \param capacity maximum number of objects in the buffer can hold
    */
    TRingBuffer(int capacity) : RingBufferBase(capacity), buffer_(getCapacity(), T()) {}

    /** Add an object to the buffer.

        \param value the value to add

        \return true if added, false if buffer was full
    */
    bool pushOn(const T& value)
	{
	    if (isFull()) return false;
	    buffer_[getWriteOffset()] = value;
	    added();
	    return true;
	}

    /** Fetch the next object from the buffer.

        \param value reference to storage to hold the object

        \return true if fetched, false if buffer was empty
    */
    bool popOff(T& value)
	{
	    if (isEmpty()) return false;
	    value = buffer_[getReadOffset()];
	    removed();
	    return true;
	}

private:
    std::vector<T> buffer_;
};

/** Specialization of TRingBuffer for memory pointer objects. Serves as a base class for typed pointer ring
    buffers.
*/
class VoidRingBuffer : public TRingBuffer<void*>
{
    using Super = TRingBuffer<void*>;
public:
    
    /** Constructor.

        \param capacity maximum number of objects in the buffer can hold
    */
    VoidRingBuffer(int capacity) : Super(capacity) {}

protected:

    /** Fetch the next object from the buffer.

        \return next value or NULL if buffer was empty
    */
    void* popOff()
	{
	    void* tmp = 0;
	    Super::popOff(tmp);
	    return tmp;
	}
};

/** Template class for ring buffers that contain pointers to user-defined types. Only contains overrides of base
    class methods with necessary type-casting to keep the classes very light-weight.
*/
template <typename T>
class PtrRingBuffer : public VoidRingBuffer
{
public:

    /** Constructor.

        \param capacity maximum number of objects in the buffer can hold
    */
    PtrRingBuffer(int capacity) : VoidRingBuffer(capacity) {}

    /** Destructor. Deletes any items in the buffer.
     */
    ~PtrRingBuffer() { while (! isEmpty()) delete popOff(); }

    /** Add an object to the buffer. Note that the buffer takes ownership of the object while it is held in the
        buffer.

        \param obj the value to add

        \return true if added, false if buffer was full
    */
    bool pushOn(T* obj) { return VoidRingBuffer::pushOn(obj); }

    /** Fetch the next object from the buffer. Hands off ownership of the object to the caller.

        \return next value or NULL if buffer was empty
    */
    T* popOff() { return static_cast<T*>(VoidRingBuffer::popOff()); }
};

} // end namespace Utils

/** \file
 */

#endif
