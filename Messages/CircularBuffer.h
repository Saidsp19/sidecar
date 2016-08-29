#ifndef SIDECAR_MESSAGES_CIRCULARBUFFER_H // -*- C++ -*-
#define SIDECAR_MESSAGES_CIRCULARBUFFER_H

#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "Messages/BinaryVideo.h"
#include "Messages/RawVideo.h"
#include "Messages/Video.h"
#include "Threading/Threading.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Messages {

class CircularBufferIterator;

/** Base class for message-specific circular buffers. A circular buffer holds a finite number of PRIMessage
    objects. The value obtained from the message PRIMessageInfo::getShaftEncoding() method determines where in
    the buffer it is stored.

    This class does all of the type-neutral work. However, it prohibits
    creation of instances. Type-specific classes provide that functionality.
*/
class CircularBuffer
{
public:
    using MessageRef = Messages::PRIMessage::Ref;

    /** Log device used by instances.

        \return log device
    */
    static Logger::Log& Log();

    /** Obtain the type enumeration of the messages held in the buffer.

        \return MetaTypeInfo enumeration value
    */
    Messages::MetaTypeInfo::Value getType() const { return type_; }

    /** Obtain the number of messages in the buffer.

        \return message count
    */
    size_t size() const { return buffer_.size(); }

    /** Determine if the buffer is void of messages.

        \return true if so
    */
    bool empty() const { return buffer_.empty(); }

    /** Remove all messages from the buffer. Iterators remain valid, but they will point to an invalid
	reference-counted value, whose get() method will return NULL.
    */
    void clear();

protected:

    /** Class utility method that installs a new CircularBuffer into the global collection of circular buffers.
        Throws an error if there is already an entry registered under the given name.

	\param name the name to register the buffer under

	\param buffer the object to register
    */
    static void Install(const std::string& name, CircularBuffer* buffer);

    /** Class utility method that returns a previously-registered CircularBuffer object. Throws an exception if
        the buffer does not exist, or if the buffer does not work with the given message type.

        \param name the name of the buffer to fetch

        \param typeInfo the message type specification the buffer must handle

        \return found CircularBuffer object
    */
    static const CircularBuffer* Get(const std::string& name,
                                     const Messages::MetaTypeInfo& typeInfo);

    /** Constructor.

        \param type type type descriptor for the messages being held
    */
    CircularBuffer(const Messages::MetaTypeInfo& type);

    /** Destructor.
     */
    ~CircularBuffer();

    /** Store a message into the buffer. The position where the message will be placed is determined by the
        value of PRIMesageInfo::getShaftEncoding().

        \param msg the message to store
    */
    void add(const MessageRef& msg);

    /** Obtain an iterator that points to the slot that contains a given message. Uses the value from
        PRIMessageInfo::getShaftEncoding(), but does not check that the actual messages are the same.

        \param msg the message to look for

        \return iterator pointing to the message
    */
    CircularBufferIterator locate(const MessageRef& msg) const;

    /** Obtain iterator to the newest element in the buffer. NOTE: if another thread adds to the the buffer,
        then this will no longer refer to the newest entry.

        \return iterator
    */
    CircularBufferIterator newest() const;

    /** Obtain iterator to the oldest element in the buffer. NOTE: if another thread adds to the the buffer,
        then this will no longer refer to the oldest entry, and instead point to newer entries.

        \return iterator
    */
    CircularBufferIterator oldest() const;

private:
    
    /** Increment and wrap an index value. Wraps the value to keep it legal as an index into the buffer.

	\param index value to increment

	\return wrapped value
    */
    size_t increment(size_t index) const;

    /** Decrement and wrap an index value. Wraps the value to keep it legal as an index into the buffer.

        \param index value to decrement

        \return wrapped value
    */
    size_t decrement(size_t index) const;

    /** Obtain the message at a given index.

        \param index location to fetch

        \return found message
    */
    MessageRef get(size_t index) const
	{ return buffer_[index == buffer_.size() ? 0 : index]; }

    using MessageVector = std::vector<MessageRef>;
    MessageVector buffer_;
    Messages::MetaTypeInfo::Value type_;
    volatile size_t oldest_;
    size_t last_;
    
    using CircularBufferMap = std::map<std::string,CircularBuffer*>;

    static Threading::Mutex::Ref collectionMutex_;
    static CircularBufferMap* collection_;

    friend class CircularBufferIterator;
};

/** Iterator for the general PRI message circular buffers. Supports forward and backward movement. Contains
    atNewest() and atOldest() methods which will return true if the iterator is located at the newest / oldest
    entry.

    The idiomatic way of using this iterator is as follows:

    \code // Visit all messages newer than a given one.
    //
    VideoCircularBuffer::iterator pos = buffer->locate(msg);
    while (! pos.atNewest()) {
    msg = *++pos;
    }
    msg = *pos;			// get the newest

    // Visit all messages older than a given one. NOTE: in a multithreading seting, this will be incorrect since
    // the oldest entry may become the newest one while inside the loop.
    //
    VideoCircularBuffer::iterator pos = buffer->locate(msg);
    VideoCircularBuffer::iterator oldest = buffer->oldest();
    while (pos != oldest) {
    msg = *--pos;
    }

    \endcode
*/
class CircularBufferIterator:
	public std::iterator<std::random_access_iterator_tag, Messages::PRIMessage::Ref, std::ptrdiff_t,
			     Messages::PRIMessage::Ref*, Messages::PRIMessage::Ref&>
{
public:
    using Super = std::iterator<std::random_access_iterator_tag, Messages::PRIMessage::Ref,
                                std::ptrdiff_t, Messages::PRIMessage::Ref*, Messages::PRIMessage::Ref&>;

    // using reference = Super::reference;
    // using value_type = Super::value_type;

    /** Determine if this iterator is the same as another one.

        \param rhs value to compare

        \return true if so
    */
    bool operator==(const CircularBufferIterator& rhs) const
	{ return buffer_ == rhs.buffer_ && index_ == rhs.index_; }

    /** Determine if this iterator is not the same as another one

        \param rhs value to compare

        \return true if so
    */
    bool operator!=(const CircularBufferIterator& rhs) const
	{ return ! operator==(rhs); }

    /** Determine if the iterator points to the oldest entry in the buffer.

        \return true if so
    */
    bool atOldest() const { return *this == buffer_->oldest(); }

    /** Determine if the iterator points to the newest entry in the buffer.

        \return true if so
    */
    bool atNewest() const { return *this == buffer_->newest(); }

protected:

    /** Default constructor. Sets up an illegal iterator. Assign a valid iterator before using.
     */
    CircularBufferIterator() : buffer_(0), index_(0) {}

    /** Constructor.

        \param buffer the buffer containing the message data

        \param index the slot in the buffer to point at
    */
    CircularBufferIterator(const CircularBuffer* buffer, int index)
	: buffer_(buffer), index_(index) {}

    /** Prefix increment operator. Moves the iterator to a newer entry, as long as the iterator is not pointing
        to the newest entry in the buffer.

        \return reference to self
    */
    void increment() { index_ = buffer_->increment(index_); }

    void moveToNewer(int distance);

    void decrement() { index_ = buffer_->decrement(index_); }

    void moveToOlder(int distance);

    /** Dereference operator. Obtain the message that the iterator points to

        \return PRIMessage reference
    */
    Messages::PRIMessage::Ref operator*() const
	{ return buffer_->get(index_); }

    /** Message invocation operator. Returns a pointer to the held message reference. Used to invoke a method on
        the held pointer.

        \return PRIMessage pointer
    */
    Messages::PRIMessage* operator->() const
	{ return buffer_->get(index_).get(); }

private:
    const CircularBuffer* buffer_;
    int index_;

    friend class CircularBuffer; ///< Allow access to the constructor.
};

template <typename T> class TCircularBuffer;

/** Type-specific iterator for a type-specific circular buffer.
 */
template <typename T>
class TCircularBufferIterator : public CircularBufferIterator
{
    using Super = CircularBufferIterator;
public:

    /** Default constructor. Sets up an illegal iterator. Assign a valid iterator before using.
     */
    TCircularBufferIterator() : Super() {}

    /** Copy constructor.

	\param it type-neutral iterator to use.
    */
    TCircularBufferIterator(const TCircularBufferIterator& it)
	: Super(it) {}

    /** Prefix increment operator. Moves the iterator to a newer entry, as long as the iterator is not pointing
        to the newest entry in the buffer.

        \return reference to self
    */
    TCircularBufferIterator& operator++()
	{ Super::increment(); return *this; }

    /** Postfix increment operator. Moves the iterator to a newer entry, as long as the iterator is not pointing
        to the newest entry in the buffer.

        \return reference to copy of self before increment
    */
    TCircularBufferIterator operator++(int)
	{ TCircularBufferIterator tmp(*this); increment(); return tmp; }

    TCircularBufferIterator& operator+=(int value)
	{ Super::moveToNewer(value); return *this; }

    TCircularBufferIterator operator+(int value) const
	{ return TCircularBufferIterator(*this) += value; }

    /** Prefix decrement operator. Moves the iterator to an older entry, as long as the iterator is not pointing
        to the oldest entry in the buffer.

        \return reference to self
    */
    TCircularBufferIterator& operator--()
	{ Super::decrement(); return *this; }

    /** Postfix decrement operator.Moves the iterator to an older entry, as long as the iterator is not pointing
        to the oldest entry in the buffer.

        \return reference to copy of self before decrement
    */
    TCircularBufferIterator operator--(int)
	{ TCircularBufferIterator tmp(*this); decrement(); return tmp; }

    TCircularBufferIterator& operator-=(int value)
	{ Super::moveToOlder(value); return *this; }
    
    TCircularBufferIterator operator-(int value) const
	{ return TCircularBufferIterator(*this) -= value; }

    /** Type-specific version of the CircularBufferIterator method.

        \return message reference 
    */
    typename T::Ref operator*() const
	{ return boost::dynamic_pointer_cast<T>(Super::operator*()); }

    /** Type-specific version of the CircularBufferIterator method.

        \return message pointer 
    */
    T* operator->() const { return dynamic_cast<T*>(Super::operator->()); }

private:

    /** Conversion constructor. Makes a type-specific iterator from a type-neutral one.

	\param it type-neutral iterator to use.
    */
    TCircularBufferIterator(const Super& it) : Super(it) {}

    friend class TCircularBuffer<T>; ///< Allow access to the constructor
};

/** Template class for a type-specific circular buffer. The template argument must provide a GetMetaTypeInfo()
    method that returns a reference to a MetaTypeInfo object that describes the messages held in the buffer.
*/
template <typename T>
class TCircularBuffer : public CircularBuffer
{
    using Super = CircularBuffer;
public:
    using const_iterator = TCircularBufferIterator<T>;

    /** Create and register a new application-wide, type-specific circular buffer object.

        \param name the name under which to register the new buffer

        \return 
    */
    static TCircularBuffer<T>* Make(const std::string& name)
	{
	    TCircularBuffer<T>* buffer = new TCircularBuffer<T>;
	    Super::Install(name, buffer);
	    return buffer;
	}

    /** Obtain a type-specific circular buffer registered under a given name.

        \param name the name to look for

        \return found type-specifiic circular buffer
    */
    static const TCircularBuffer<T>* Get(const std::string& name)
	{
	    return static_cast<const TCircularBuffer<T>*>(Super::Get(name, T::GetMetaTypeInfo()));
	}

    /** Type-specific version of CircularBuffer::locate() method.

        \param msg add a message to the buffer
    */
    void add(const typename T::Ref& msg) { Super::add(msg); }

    /** Type-specific version of CircularBuffer::locate() method.

        \param msg the message to look for

        \return type-specific iterator pointing to the given message
    */
    TCircularBufferIterator<T> locate(const typename T::Ref& msg) const
	{ return TCircularBufferIterator<T>(Super::locate(msg)); }

    /** Obtain iterator to the newest element in the buffer. NOTE: if another thread may add to the the buffer,
        then this will no longer refer to the newest entry.

        \return iterator
    */
    TCircularBufferIterator<T> newest() const
	{ return TCircularBufferIterator<T>(Super::newest()); }

    /** Obtain iterator to the oldest element in the buffer. NOTE: if another thread may add to the the buffer,
        then this will no longer refer to the oldest entry, and instead point to newer entries.

        \return iterator
    */
    TCircularBufferIterator<T> oldest() const
	{ return TCircularBufferIterator<T>(Super::oldest()); }

private:

    /** Default constructor.
     */
    TCircularBuffer() : Super(T::GetMetaTypeInfo()) {}

    /** Destructor.
     */
    ~TCircularBuffer() {}

};

using BinaryVideoCircularBuffer = TCircularBuffer<Messages::BinaryVideo>;
using RawVideoCircularBuffer = TCircularBuffer<Messages::RawVideo>;
using VideoCircularBuffer = TCircularBuffer<Messages::Video>;

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
