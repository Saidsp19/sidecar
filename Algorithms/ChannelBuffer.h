#ifndef SIDECAR_ALGORITHMS_CHANNELBUFFER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CHANNELBUFFER_H

#include <deque>
#include <vsip/matrix.hpp>
#include <vsip/vector.hpp>

#include "Algorithms/Processor.h"
#include "Messages/PRIMessage.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

using ComplexType = std::complex<float>;
using VsipComplexVector = vsip::Vector<ComplexType>;
using VsipComplexMatrix = vsip::Matrix<ComplexType>;

class ManyInAlgorithm;

/** Abstract base class of an input message buffer that will hold a maximum number of messages in FIFO order.
    Held messages are of type PRIMessage; a derived template class, TChannelBuffer, works with concrete SideCar
    messages types.

    Note that this class works with a ManyInAlgorithm instance, requiring one in its constructor. After a
    ChannelBuffer places a new message in its queue, it calls the ManyInAlgorithm::processMessageReceived()
    method to notify the ManyInAlgorithm object of new data.
*/
class ChannelBuffer {
public:
    using Container = std::deque<Messages::PRIMessage::Ref>;
    using const_iterator = Container::const_iterator;

    /** Create generic short name for the input channel. Given an index value of N, this will return a string
        with the format "input#Enabled", with # replaced by the value N+1.

        \param index the channel index to use

        \return generic short name
    */
    static std::string MakeGenericShortName(int index);

    /** Create generic long name for the input channel. Given an index value of N, this will return a string
        with the format "Input # Enabled", with # replaced by the value N+1.

        \param index the channel index to use

        \return generic short name
    */
    static std::string MakeGenericLongName(int index);

    /** Constructor.

        \param processor object to notify when new data arrives in the buffer

        \param channelIndex the channel index unique to this object

        \param maxBufferSize the maximum number of messages to hold
    */
    ChannelBuffer(ManyInAlgorithm& processor, size_t channelIndex, size_t maxBufferSize);

    /** Destructor. Properly dispose of any held messages.
     */
    virtual ~ChannelBuffer();

    /** Obtain the channel index for this object

        \return channel index
    */
    size_t getChannelIndex() const { return channelIndex_; }

    /** Obtain the maximum number of messages to hold

        \return buffer limit
    */
    size_t getMaxBufferSize() const { return maxBufferSize_; }

    /** Obtain the number of messages in the buffer

        \return buffer size
    */
    size_t size() const { return container_.size(); }

    /** Determine if the buffer is empty

        \return true if so
    */
    bool isEmpty() const { return container_.empty(); }

    /** Determine if the buffer is enabled. A disabled buffer ignores all addData() calls.

        \return true if so
    */
    bool isEnabled() const { return enabled_; }

    /** Obtain the sequence counter of the first message in the queue. NOTE: if the queue is empty, this returns
        0.

        \return sequence counter
    */
    uint32_t getNextSequenceCounter() const { return nextSequenceCounter_; }

    /** Remove messages from the front of the queue whose sequence counter values are less than the given value

        \param sequenceCounter the value to compare against

        \return true if the buffer has a message with the given sequence counter
    */
    bool pruneToSequenceCounter(uint32_t sequenceCounter);

    /** Change the maximum number of messages to hold.

        \param maxBufferSize new value
    */
    virtual void setMaxBufferSize(size_t maxBufferSize);

    /** Change the enabled state.

        \param value new value
    */
    virtual void setEnabled(bool value);

    /** Reset the buffer by forgetting any held messagess.
     */
    virtual void reset();

    /** Obtain a slide of the data, copying it into a VSIPL vector. NOTE: only valid if isEmpty() is false.
        Always works on the first message in the FIFO queue.

        \param offset index of the first sample value to use

        \param span count of sample values to gather

        \param slice container to hold the resulting complex values

        \param doPop if true, remove the first message from the queue after
        working with it
    */
    virtual void getSlice(int offset, int span, VsipComplexVector& slice, bool doPop) = 0;

    /** Add a message to the queue.

        \param msg the message to process

        \return true if successful
    */
    bool addData(const Messages::PRIMessage::Ref& msg);

    /** Create a runtime parameter to control the channel enabled state. The created Parameter::BoolValue object
        will have names created from MakeGenericShortName() and MakeGenericLongName().

        \return true if successful
    */
    virtual bool makeEnabledParameter();

    /** Create a runtime parameter to control the channel enabled state.

        \param shortName

        \param longName

        \return true if successful
    */
    virtual bool makeEnabledParameter(const std::string& shortName, const std::string& longName);

    /** Obtain read-only iterator to the first sample value in the message.

        \return read-only iterator
    */
    const_iterator begin() const { return container_.begin(); }

    /** Obtain read-only iterator to the last + 1 sample value in the message.

        \return read-only iterator
    */
    const_iterator end() const { return container_.end(); }

    /** Obtain the oldest message in the queue. NOTE: only call if the queue is not empty.

        \return Messages::PRIMessage reference
    */
    Messages::PRIMessage::Ref getFront() const { return container_.front(); }

    /** Remove the oldest message in the queue. NOTE: only call if the queue is not empty.
     */
    Messages::PRIMessage::Ref popFront()
    {
        if (isEmpty()) return Messages::PRIMessage::Ref();
        Messages::PRIMessage::Ref msg(getFront());
        popFrontInternal();
        return msg;
    }

protected:
    /** Remove the oldest message from the queue. Updates the nextSequenceCounter_ value. NOTE: only call if the
        queue is not empty.
    */
    void popFrontInternal();

    /** Remove messages until the size of the buffer is less than or equal to maxBufferSize_.
     */
    void pruneToMaxBufferSize();

    /** Callback invoked when the 'enabled' parameter state changes.

        \param parameter the parameter that changed
    */
    virtual void enabledChanged(const Parameter::BoolValue& parameter);

private:
    ManyInAlgorithm& processor_;
    Parameter::BoolValue::Ref enabledParam_;
    size_t channelIndex_;
    size_t maxBufferSize_;
    uint32_t nextSequenceCounter_;
    Container container_;
    bool enabled_;
};

/** Templated class for a input channel buffer. Primary use is for algorithms that take in more than one input
    channel. It provides an API that allows the algorithm to obtain messsages across all channels synchronized
    by their sequence number.`
*/
template <typename T>
class TChannelBuffer : public ChannelBuffer, public TProcessor<TChannelBuffer<T>, T> {
public:
    /** Constructor. Registers ourselves with the owner as the processor for the channel.

        \param master the algorithm doing the processing

        \param channelIndex the index of the channel we are processing

        \param maxBufferSize the maximum number of messages to queue
    */
    TChannelBuffer(ManyInAlgorithm& owner, size_t channelIndex, size_t maxBufferSize);

    /** Obtain the oldest message in the queue. NOTE: only call if the queue is not empty.

        \return T reference
    */
    typename T::Ref getFront() const
    {
        if (isEmpty()) return typename T::Ref();
        return boost::dynamic_pointer_cast<T>(ChannelBuffer::getFront());
    }

    /** Remove the oldest message in the queue return it. NOTE: only call if the queue is not empty.

        \return T reference
    */
    typename T::Ref popFront()
    {
        if (isEmpty()) return typename T::Ref();
        typename T::Ref msg(getFront());
        popFrontInternal();
        return msg;
    }

    /** Obtain a slide of the data, copying it into a VSIPL vector. NOTE: only valid if isEmpty() is false.
        Always works on the first message in the FIFO queue. Implementation of ChannelBuffer API.

        Treats the message data as interleved I/Q data. Creates complex values
        from subsequent values and adds them to the given VSIPL vector
        container. If the message is too short to accomodate the span
        parameter, pads the VSIPL vector with 0,0 complex values.

        \param offset index of the first sample value to use

        \param span count of sample values to gather

        \param slice container to hold the resulting complex values

        \param doPop if true, remove the first message from the queue after
        working with it
    */
    void getSlice(int offset, int span, VsipComplexVector& slice, bool doPop)
    {
        if (isEmpty()) return;

        // NOTE: important to hold reference to message in case we pop it from the deque.
        //
        typename T::Ref msg(getFront());
        typename T::Container& container(msg->getData());
        if (doPop) popFront();

        // Make sure that offset + span is no greater than the size of the message.
        //
        int limit = span;
        if ((offset + span) * 2 > int(container.size())) {
            limit = container.size() / 2 - offset;
            if (limit < 0) limit = 0;
        }

        // Now copy over the data as complex values.
        //
        typename T::DatumType* ptr = &container[offset * 2];
        for (int index = 0; index < limit; ++index, ptr += 2) { slice.put(index, ComplexType(*ptr, *(ptr + 1))); }

        // Make sure that the slices we return are of the requested size, even if our message is too short.
        //
        while (limit < span) { slice.put(limit++, ComplexType()); }
    }
};

} // end namespace Algorithms
} // end namespace SideCar

#include "Algorithms/ManyInAlgorithm.h"

namespace SideCar {
namespace Algorithms {

template <typename _T>
TChannelBuffer<_T>::TChannelBuffer(ManyInAlgorithm& owner, size_t channelIndex, size_t maxBufferSize) :
    ChannelBuffer(owner, channelIndex, maxBufferSize), TProcessor<TChannelBuffer<_T>, _T>(this, &ChannelBuffer::addData)
{
    owner.addProcessor(channelIndex, _T::GetMetaTypeInfo(), this);
}

} // end namespace Algorithms
} // end namespace SideCar

#endif
