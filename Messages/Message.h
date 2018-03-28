#ifndef SIDECAR_MESSAGES_MESSAGE_H // -*- C++ -*-
#define SIDECAR_MESSAGES_MESSAGE_H

#include "boost/iterator/iterator_facade.hpp"
#include "boost/shared_ptr.hpp"

#include "IO/CDRStreamable.h"
#include "IO/Printable.h"
#include "Messages/MessageImpl.h"

namespace SideCar {
namespace Messages {

class MetaTypeInfo;

/** Base class for all message containers. Implements the same interface as BaseMessageImpl, but simply forwards
    requests to the held BaseMessageImpl object. Type-specific Derived classes must define
    getMessageMetaTypeInfo() to return the type-specific meta data.
*/
class BaseMessage : public IO::CDRStreamable<BaseMessage>, public IO::Printable<BaseMessage> {
    using Impl = BaseMessageImpl;

public:
    /** Default constructor. Creates a 'null' instance with no backing implementation.
     */
    BaseMessage() : impl_() {}

    /** Determine if there is a valid backing object present.

        \return true if so
    */
    bool isValid() const { return impl_.get() != nullptr; }

    /** Obtain meta data that describes the message type. Derived classes must implement.

        \return reference to Messages::MetaTypeInfo object
    */
    virtual const Messages::MetaTypeInfo& getMessageMetaTypeInfo() const { return impl_->getMessageMetaTypeInfo(); }

    /** Obtain the name of the entity that created this message; usually the name of an algorithm.

        \return creator name
    */
    const std::string& getMessageProducer() const { return impl_->getMessageProducer(); }

    /** Obtain the sequence number associated with this message.

        \return sequence number
    */
    uint32_t getMessageSequenceNumber() const { return impl_->getMessageSequenceNumber(); }

    /** Set the sequence number associated with this message.

        \param value new sequence number to use
    */
    void setMessageSequenceNumber(uint32_t value) { impl_->setMessageSequenceNumber(value); }

    /** Obtain the time when this message was created

        \return creation Time::TimeStamp value
    */
    Time::TimeStamp getMessageCreatedTimeStamp() const { return impl_->getMessageCreatedTimeStamp(); }

    /** Set the time when this message was created.

        \param value new value to use
    */
    void setMessageCreatedTimeStamp(const Time::TimeStamp& value) { impl_->setMessageCreatedTimeStamp(value); }

    /** Obtain the time when this message was emitted to a network device. Useful in identifying latencies among
        nodes.

        \return Time::TimeStamp value
    */
    Time::TimeStamp getMessageEmittedTimeStamp() const { return impl_->getMessageEmittedTimeStamp(); }

    /** Obtain the number of bytes occupied by this message.

        \return byte count
    */
    size_t getMessageSize() const { return impl_->getMessageSize(); }

    /** Update attribute values using data from an ACE input CDR object.

        \param cdr data source to use

        \return reference to given ACE_InputCDR object
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr) { return impl_->load(cdr); }

    /** Store attribute values into an ACE output CDR object.

        \param cdr data sink to use

        \return reference to given ACE_OutputCDR object
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const { return impl_->write(cdr); }

    /** Write out a textual representation of the object to the given stream.

        \param os the C++ output stream to write to

        \return reference to given output stream
    */
    std::ostream& print(std::ostream& os) const { return impl_->print(os); }

protected:
    /** Constructor. Restrict access to derived classes in order to guarantee type-safety while using getImpl()
        type-cast methods.

        \param impl implementation object to use for data
    */
    BaseMessage(BaseMessageImpl* impl) : impl_(impl) {}

    /** Template type-cast operator to obtain a modifiable reference type derived from BaseMessageImpl. NOTE:
        should cause code to abort if the held pointer is NULL.

        \return modifiable reference to held object of type T
    */
    template <typename T>
    T& getTImpl()
    {
        return *dynamic_cast<T*>(impl_.get());
    }

    /** Template type-cast operator to obtain a read-only reference type derived from BaseMessageImpl. NOTE:
        should cause code to abort if the held pointer is NULL.

        \return read-only reference to held object of type T
    */
    template <typename T>
    const T& getTImpl() const
    {
        return *dynamic_cast<const T*>(impl_.get());
    }

private:
    /** Implementation object that provides the actual data.
     */
    boost::shared_ptr<BaseMessageImpl> impl_;
};

/** Class for all sampling message containers. Implements the same type-agnostic interface as
    SamplingMessageImpl, but simply forwards requests to the held SamplingMessageImpl object. Provides
    additional convenience methods for working with sampling data.
*/
class SamplingMessage : public BaseMessage {
    using Super = BaseMessage;
    using Impl = SamplingMessageImpl;

public:
    /** Optain the state flags from the sampling system.

        \return state flags
    */
    uint32_t getSamplingFlags() const { return getImpl().getSamplingFlags(); }

    /** Obtain the high-frequency time stamp from the sampling system.

        \return high-frequency time stamp
    */
    uint32_t getSamplingTimeStamp() const { return getImpl().getSamplingTimeStamp(); }

    /** Obtain the PRI sequence counter from the sampling system.

        \return PRI sequence counter
    */
    uint32_t getSamplingSequenceCounter() const { return getImpl().getSamplingSequenceCounter(); }

    /** Obtain the radar shaft encoding value from the sampling system.

        \return shaft encoding
    */
    uint32_t getSamplingShaftEncoding() const { return getImpl().getSamplingShaftEncoding(); }

    /** Obtain the PRF encoding value from the sampling system.

        \return PRF encoding
    */
    uint32_t getSamplingPRFEncoding() const { return getImpl().getSamplingPRFEncoding(); }

    /** Obtain the IRIG time value from the sampling system.

        \return IRIG time
    */
    double getSamplingIRIGTime() const { return getImpl().getSamplingIRIGTime(); }

    /** Obtain the range value of the first sample from the sampling system

        \return range of sample 0
    */
    double getSamplingRangeMin() const { return getImpl().getSamplingRangeMin(); }

    /** Obtain the delta range between adjacent samples

        \return delta range between samples
    */
    double getSamplingRangeFactor() const { return getImpl().getSamplingRangeFactor(); }

    /** Obtain the range value for a given sample

        \param index location of the sample gate to use

        \return range value
    */
    double getSamplingRangeAt(size_t index) const { return index * getSamplingRangeFactor() + getSamplingRangeMin(); }

    /** Obtain the number of sample values in the message.

        \return sample count
    */
    size_t getSamplingSize() const { return getImpl().getSamplingSize(); }

    /** Obtain an azimuth value in radians for the sampling message, based on the shaft encoder value from the
        RIU. NOTE: assumes that the radar is pointing north when getSamplingShaftEncoding() == 0.

        \return azimuth in radians
    */
    double getAzimuthStart() const;

    /** Obtain an azimuth value in radians for the sampling message, based on the shaft encoder and the north
        mark values from the VME syatem.

        \return azimuth in radians
    */
    double getAzimuthEnd() const;

protected:
    /** Constructor. Restrict access to derived classes in order to guarantee type-safety while using getImpl()
        type-cast methods.

        \param impl implementation object to use for data
    */
    SamplingMessage(Impl* impl) : Super(impl) {}

private:
    /** Type-cast operator to obtain a modifiable reference type derived from SamplingMessageImpl.

        \return modifiable reference to held object of type SamplingMessageImpl
    */
    Impl& getImpl() { return getTImpl<Impl>(); }

    /** Type-cast operator to obtain a read-oonly reference type derived from SamplingMessageImpl.

        \return read-only reference to held object of type SamplingMessageImpl
    */
    const Impl& getImpl() const { return getTImpl<Impl>(); }
};

/** Iterator definition for TSamplingMessage classes. Provides functionality of a random-access iterator. Can
    create mutable or read-only iterators depending on the value of TDatumType.
*/
template <typename TDatumType>
class TSamplingMessageIterator : public boost::iterator_facade<TSamplingMessageIterator<TDatumType>, TDatumType,
                                                               boost::bidirectional_traversal_tag> {
    using Self = TSamplingMessageIterator<TDatumType>;

public:
    /** Default constructor.
     */
    TSamplingMessageIterator() : ptr_(0) {}

    /** Initializing constructor.

        \param ptr TDatumType pointer to use for iterator
    */
    explicit TSamplingMessageIterator(TDatumType* ptr) : ptr_(ptr) {}

    /** Conversion constructor. Performs conversion from one iterator type to another as long as the
        corresponding TDatumType template types are compatible; a compile failure will result for incompatible
        types

        \param rhs iterator to convert
    */
    template <typename TOther>
    TSamplingMessageIterator(TSamplingMessageIterator<TOther> const& rhs) : ptr_(rhs.ptr_)
    {
    }

private:
    /** Grant friend access so that boost iterator methods can access our implementation methods below.
     */
    friend class boost::iterator_core_access;

    /** Implementation of boost::iterator_facade method that provides a value from an interator.

        \return mutable reference to value that iterator points to
    */
    TDatumType& dereference() const { return *ptr_; }

    /** Determine if this iterator is the same as another.

        \param rhs other iterator to compare with

        \return true if the same
    */
    template <typename TOther>
    bool equal(const TSamplingMessageIterator<TOther>& rhs) const
    {
        return ptr_ == rhs.ptr_;
    }

    /** Move the iterator to the next value position
     */
    void increment() { ++ptr_; }

    /** Move the iterator to the previous value position
     */
    void decrement() { --ptr_; }

    /** Change the iterator by a positive or negative offset

        \param value the offset to apply
    */
    void advance(ptrdiff_t value) { ptr_ += value; }

    /** Determine how much this iterator must advance to reach another one

        \param rhs other iterator to compare with

        \return offset
    */
    ptrdiff_t distance_to(const Self& rhs) const { return rhs.ptr_ - ptr_; }

    /** Grant friend access to other TSamplingMessageIterator derivations so they can access our ptr_ attribute.
     */
    template <typename>
    friend class TSamplingMessageIterator;

    TDatumType* ptr_; ///< Pointer to sequence elements
};

/** Template class for all type-specific sampling messages. Supports iterator and indexed access to underlying
    sample data. Attempts to model std::vector in functionality.
*/
template <typename TDatumType>
class TSamplingMessage : public SamplingMessage {
    using Super = SamplingMessage;
    using Impl = TSamplingMessageImpl<TDatumType>;

public:
    using iterator = TSamplingMessageIterator<TDatumType>;
    using const_iterator = TSamplingMessageIterator<TDatumType const>;

    /** Constructor. Restrict access to derived classes in order to guarantee type-safety while using getImpl()
        type-cast methods.

        \param impl implementation object to use for data
    */
    TSamplingMessage(Impl* impl) : Super(impl) {}

    /** Obtain a mutable reference to a specific sample value

        \param index location of the sample

        \return mutable reference to the value
    */
    TDatumType& operator[](size_t index) { return getImpl()[index]; }

    /** Obtain a read-only reference to a specific sample value

        \param index location of the sample

        \return read-only reference to the value
    */
    const TDatumType& operator[](size_t index) const { return getImpl()[index]; }

    /** Clear the sequence of all samples.
     */
    void clear() { getImpl().clear(); }

    /** Add a sample value to the end of the sequence.

        \param v new value to add
    */
    void push_back(const TDatumType& v) { getImpl().push_back(v); }

    /** Obtain mutable iterator that points to the first sample in the sequence.

        \return mutable iterator at sample 0
    */
    iterator begin() { return iterator(getSamples()); }

    /** Obtain mutable iterator that points to the location following the last sample in the sequence.

        \return mutable iterator at (non-existant) sample N
    */
    iterator end() { return iterator(getSamples() + getSamplingSize()); }

    /** Obtain read-only iterator that points to the first sample in the sequence.

        \return read-only iterator at sample 0
    */
    const_iterator begin() const { return const_iterator(getSamples()); }

    /** Obtain read-only iterator that points to the location following the last sample in the sequence.

        \return read-only iterator at (non-existant) sample N
    */
    const_iterator end() const { return const_iterator(getSamples() + getSamplingSize()); }

    void append(const_iterator begin, const_iterator end)
    {
        while (begin != end) push_back(*begin++);
    }

protected:
    /** Obtain a pointer to the first sample value. NOTE: use with caution!

        \return pointer to first sample
    */
    TDatumType* getSamples() { return getImpl().getSamples(); }

    /** Obtain a read-only pointer to the first sample value. NOTE: use with caution!

        \return read-only pointer to first sample
    */
    const TDatumType* getSamples() const { return getImpl().getSamples(); }

private:
    /** Type-cast operator to obtain a modifiable reference type derived from TSamplingMessageImpl<TDatumType>.

        \return modifiable reference to held object of type TSamplingMessageImpl<TDatumType>
    */
    Impl& getImpl() { return getTImpl<Impl>(); }

    /** Type-cast operator to obtain a read-only reference type derived from TSamplingMessageImpl<TDatumType>.

        \return read-only reference to held object of type TSamplingMessageImpl<TDatumType>
    */
    const Impl& getImpl() const { return getTImpl<Impl>(); }
};

/** Derivation of TSamplingMessage for messages with 16-bit values.
 */
class ShortSequence : public TSamplingMessage<short> {
public:
    const MetaTypeInfo& GetMetaTypeInfo();

    ShortSequence(const std::string& producer, const MetaTypeInfo& implType);

    ShortSequence(const std::string& producer, const SamplingMessage& basis);

private:
    static MetaTypeInfo metaTypeInfo_;
};

/** Derivation of TSamplingMessage for messages with 2-tuple (complex) 16-bit values.
 */
class IQSequence : public TSamplingMessage<short> {
public:
    const MetaTypeInfo& GetMetaTypeInfo();

    IQSequence(const std::string& producer, const MetaTypeInfo& implType);

    IQSequence(const std::string& producer, const SamplingMessage& basis);

private:
    static MetaTypeInfo metaTypeInfo_;
};

/** Derivation of TSamplingMessage for messages with 8-bit (boolean) values, where zero is false and non-zero is
    true. Using 8-bit booleans for simplicity.
*/
class BooleanSequence : public TSamplingMessage<char> {
public:
    const MetaTypeInfo& GetMetaTypeInfo();

    BooleanSequence(const std::string& producer, const MetaTypeInfo& implType);

    BooleanSequence(const std::string& producer, const SamplingMessage& basis);

private:
    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace SideCar

#endif
