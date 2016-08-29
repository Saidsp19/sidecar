#ifndef SIDECAR_MESSAGES_MESSAGEIMPL_H // -*- C++ -*-
#define SIDECAR_MESSAGES_MESSAGEIMPL_H

#include "Time/TimeStamp.h"

class ACE_InputCDR;
class ACE_OutputCDR;

namespace SideCar {
namespace Messages {

class MetaTypeInfo;

/** Virtual interface that all messsage implementations must support. Provides general information about the
    message: its type, creator, and time of creation and emission. Message implementations must provide
    definitions for the abstract virtual functions below.
*/
class BaseMessageImpl
{
public:

    virtual ~BaseMessageImpl() {}

    virtual const MetaTypeInfo& getMessageMetaTypeInfo() const = 0;

    virtual const std::string& getMessageProducer() const = 0;

    virtual uint32_t getMessageSequenceNumber() const = 0;

    virtual void setMessageSequenceNumber(uint32_t) = 0;

    virtual Time::TimeStamp getMessageCreatedTimeStamp() const = 0;

    virtual void setMessageCreatedTimeStamp(const Time::TimeStamp&) = 0;

    virtual Time::TimeStamp getMessageEmittedTimeStamp() const = 0;

    virtual void setMessageEmittedTimeStamp(const Time::TimeStamp&) = 0;

    virtual size_t getMessageSize() const = 0;

    virtual ACE_InputCDR& load(ACE_InputCDR& cdr) = 0;

    virtual ACE_OutputCDR& write(ACE_OutputCDR& cdr) const = 0;

    virtual std::ostream& print(std::ostream& os) const = 0;
};

/** Interface that all sampling message implementations must support. Provides type-independent information
    specific to the sampling event: high-frequency counter, PRI counter, shaft encoding value, PRF encoding
    value, IRIG time, range constants for linear conversion from sample offset to range, and the number of
    samples.

    Derived type-specific sampling messages provide access to the actual sample
    values.
*/
class SamplingMessageImpl : public BaseMessageImpl
{
public:

    virtual uint32_t getSamplingFlags() const  = 0;

    virtual uint32_t getSamplingTimeStamp() const = 0;

    virtual uint32_t getSamplingSequenceCounter() const = 0;

    virtual uint32_t getSamplingShaftEncoding() const = 0;

    virtual uint32_t getSamplingPRFEncoding() const = 0;

    virtual double getSamplingIRIGTime() const = 0;

    virtual double getSamplingRangeMin() const = 0;

    virtual double getSamplingRangeFactor() const = 0;

    virtual size_t getSamplingSize() const = 0;

    double getRangeAt(size_t index) const
	{ return index * getSamplingRangeFactor() + getSamplingRangeMin(); }
};

/** Template interface that provides access to sample values of a sampling message.
 */
template <typename TDatumType>
class TSamplingMessageImpl : public SamplingMessageImpl
{
public:

    using DatumType = TDatumType;

    virtual TDatumType* getSamples() = 0;

    virtual const TDatumType* getSamples() const = 0;

    virtual TDatumType& operator[](size_t index) = 0;

    virtual const TDatumType& operator[](size_t index) const = 0;

    virtual void clear() = 0;

    virtual void push_back(TDatumType value) = 0;
};

} // end namespace Messages
} // end namespace SideCar

#endif
