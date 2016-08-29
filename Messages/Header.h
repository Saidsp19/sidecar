#ifndef SIDECAR_MESSAGES_HEADER_H // -*- C++ -*-
#define SIDECAR_MESSAGES_HEADER_H

#include <string>
#include "boost/shared_ptr.hpp"

#include "IO/CDRStreamable.h"
#include "IO/Printable.h"
#include "Messages/GUID.h"
#include "Time/TimeStamp.h"

class QDomElement;

namespace Logger { class Log; }

namespace SideCar {
namespace Messages {

class MetaTypeInfo;

/** Definition of the header found at the beginning of all messages, regardless of content type. All messages
    contain a timestmp indicating when they were created, a timestamp when they were written out to a file or
    the network, and an unique sequence number.
*/
class Header : public IO::CDRStreamable<Header>, public IO::Printable<Header>
{
public:
    using Ref = boost::shared_ptr<Header>;

    /** Obtain log device to use for objects of this class.

        \return log device
    */
    static Logger::Log& Log();

    static const MetaTypeInfo* GetMessageMetaTypeInfo(ACE_InputCDR& cdr);

    /** Constructor for messages read in from an external device.

        \param metaTypeInfo type specification for the message
    */
    explicit Header(const MetaTypeInfo& metaTypeInfo);

    /** Constructor for new messages

        \param producer name of the entity that created the message

        \param metaTypeInfo type specification for the message
    */
    Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo);

    /** Constructor for new messages

        \param producer name of the entity that created the message

        \param metaTypeInfo type specification for the message
    */
    Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const Ref& basis);

    /** Constructor for new messages

        \param producer name of the entity that created the message

        \param metaTypeInfo type specification for the message
    */
    Header(const std::string& producer, const MetaTypeInfo& metaTypeInfo, const Ref& basis,
           MetaTypeInfo::SequenceType sequenceNumber);

    /** Destructor. Made virtual so that derived classes will properly clean up our instance values
	(specifically guid_) destructed thru a Header pointer.
    */
    virtual ~Header();

    /** Obtain the type specification for the message.

        \return 
    */
    const MetaTypeInfo& getMetaTypeInfo() const { return metaTypeInfo_; }

    /** Get the globally-unique ID of the message. A GUID contains a unique sequence number given to it by the
        metaTypeInfo_ attribute, accessed by the GUID.getMessageSequenceNumber().

        \return unique ID
    */
    const GUID& getGloballyUniqueID() const { return guid_; }

    /** Get the message sequence number for the message. This is the sequence number from the GUID. Note that
        alone this is not unique among all SideCar messages; it is only unique within a message type.

        \return sequence number
    */
    MetaTypeInfo::SequenceType getMessageSequenceNumber() const { return guid_.getMessageSequenceNumber(); }

    /** Set the sequence number for this message. Not to be used lightly. Particularly useful when an algorithm
        generates multiple output messages that should be synchronized by their sequence number.

	NOTE: a better approach than this would be to have unique message sequence generators, which is
        available right now since there is a unique sequence generator associated with each unique producer
        name.

        \param msn new sequence number to use
    */
    void setMessageSequenceNumber(MetaTypeInfo::SequenceType msn) { guid_.setMessageSequenceNumber(msn); }

    /** Obtain when the message was created.

        \return time stamp reference
    */
    const Time::TimeStamp& getCreatedTimeStamp() const { return createdTimeStamp_; }

    /** Revise the creation message timestamp. Used by some SideCar utilities that wish to set/adjust the
        message frequency, which is determined by this time stamp.

        \param timeStamp new time stamp value

	\return old value
    */
    Time::TimeStamp setCreatedTimeStamp(const Time::TimeStamp& timeStamp);

    /** Obtain when the message was written to an external device (file or network). Used by external utilities
        to calculate processing latencies within algorithms and processing chains.

        \return time stamp reference
    */
    const Time::TimeStamp& getEmittedTimeStamp() const { return emittedTimeStamp_; }

    /** Set the emitted time stamp with the given value.

        \param timeStamp the time value to assign

        \return previous emitted time stamp value
    */
    Time::TimeStamp setEmittedTimeStamp(const Time::TimeStamp& timeStamp);

    /** Obtain the C++ structure size for this object. Derived classes must override if they extend Header with
        additional members.

        \return size of a Header object.
    */
    virtual size_t getSize() const { return sizeof(Header); }

    /** Obtain new instance data from a CDR input stream

        \param cdr stream to read from

        \return stream read from
    */
    virtual ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write out the header to a CDR output stream. NOTE: by design we only write out our time stamp -- the
        byte order and size are handled by one of the IO::Sink classes.

        \param cdr CDR stream to write to

        \return CDR stream written to
    */
    virtual ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    /** Utility functor that inserts a textual representation of a Header's header data into a std::ostream
	object. Example of it use: \code LOGDEBUG << msg.headerPrinter() << std::endl; \endcode Relies on
	Header::printHeader() to do the actual conversion of header information to text, which derived classes
	may override.
    */
    struct HeaderPrinter
    {
	const Header& ref_;
	HeaderPrinter(const Header& ref) : ref_(ref) {}
	friend std::ostream& operator<<(std::ostream& os, const HeaderPrinter& us)
            { return us.ref_.printHeader(os); }
    };

    /** Convenience method that simply returns a new HeaderPrinter functor for this header object.

        \return HeaderPrinter object
    */
    HeaderPrinter headerPrinter() const { return HeaderPrinter(*this); }

    /** Utility functor that inserts a textual representation of message data into a std::ostream object.
	Example of it use: \code LOGDEBUG << msg.dataPrinter() << std::endl; \endcode Relies on
	Header::printData() to do the actual conversion of data to text, which derived classes may override.
    */
    struct DataPrinter
    {
	const Header& ref_;
	DataPrinter(const Header& ref) : ref_(ref) {}
	friend std::ostream& operator<<(std::ostream& os, const DataPrinter& us)
            { return us.ref_.printData(os); }
    };

    /** Convenience method that simply returns a new DataPrinter functor for this header object.

        \return DataPrinter object
    */
    DataPrinter dataPrinter() const { return DataPrinter(*this); }

    /** Utility functor that inserts an XML representation of message data into a std::ostream object. Example
	of it use: \code LOGDEBUG << msg.xmlPrinter() << std::endl; \endcode Relies on Header::printData() to do
	the actual conversion of data to text, which derived classes may override.
    */
    struct XMLPrinter
    {
	const Header& ref_;
	XMLPrinter(const Header& ref) : ref_(ref) {}
	friend std::ostream& operator<<(std::ostream& os, const XMLPrinter& us)
            { return us.ref_.printXML(os); }
    };

    /** Convenience method that simply returns a new XMLPrinter functor for this header object.

        \return XMLPrinter object
    */
    XMLPrinter xmlPrinter() const { return XMLPrinter(*this); }

    /** Write out the entire message to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& print(std::ostream& os) const;

    /** Write out the message header to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printHeader(std::ostream& os) const;

    /** Write out the message data to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printData(std::ostream& os) const;

    /** Write out the message to a C++ text output stream in XML format. Note: invokes printDataXML() to write
        the data elements of the message.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printXML(std::ostream& os) const;

    /** Write out the message data to a C++ text output stream in XML format.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& printDataXML(std::ostream& os) const;

    /** Determine if the message has a basis message attached to it.

        \return true if so
    */
    bool hasBasis() const { return basis_.get() != 0; }

    /** Obtain a reference to the message that this one was based on. NOTE: currently, this is only valid within
        a processing stream; in orther words, once a message is written to disk or the network, the message
        loses its basis value.

	\return message reference
    */
    Ref getBasis() const { return basis_; }

    /** Templated version of the above method. Returns a typed reference to the basis message. Returns an
        invalid reference if the type-cast is invalid.

        \return typed message reference
    */
    template <typename T>
    typename T::Ref getBasis() const { return boost::dynamic_pointer_cast<T>(basis_); }

protected:

    /** Obtain new instance data from an XML input stream.

        \param xsr stream to read from
    */
    virtual void loadXML(XmlStreamReader& xsr);

private:

    /** Constructor restricted to use for the GetMessageMetaTypeInfo() class method.

        \param cdr the input byte stream to decode
    */
    Header(ACE_InputCDR& cdr);

    /** Prohibit copy construction.

        \param rhs copy to use
    */
    Header(const Header& rhs);

    const MetaTypeInfo& metaTypeInfo_; ///< Meta type info for this object
    GUID guid_;			       ///< Globally-unique ID for this object
    Time::TimeStamp createdTimeStamp_; ///< When created
    mutable Time::TimeStamp emittedTimeStamp_; ///< When emitted
    Ref basis_;			       ///< Msg that is the basis for this one

    /** Header v1 loader. Reads in GUID and created timestamp.

        \param object the Header object to load into

        \param cdr the input stream to read from

        \return input stream read from
    */
    static ACE_InputCDR& LoadV1(Header* object, ACE_InputCDR& cdr);

    /** Header v2 loader. V2 contains an emitted timestamp for the message to better measure and
        identify latency issues.

        \param object the Header object to load into

        \param cdr the input stream to read from

        \return input stream read from
    */
    static ACE_InputCDR& LoadV2(Header* object, ACE_InputCDR& cdr);

    /** Factory method that creates and returns an array of supported versioned loaders.

        \return 
    */
    static TLoaderRegistry<Header>::VersionedLoaderVector DefineLoaders();
    static TLoaderRegistry<Header> loaderRegistry_;
};

/** Convenience operator overload that inserts a textual representation of a message's header and data into a
    std::ostream object. Example of its use: \code LOGDEBUG << msg << std::endl; \endcode Relies on
    Header::print() to do the actual conversion of data to text, which derived classes may override. To just
    show header or data, use the output stream inserters printHeader or printData.

    \param os stream to write to

    \param ref Header reference to print

    \return stream written to
*/
inline std::ostream&
operator<<(std::ostream& os, const Header::Ref& ref)
{ return ref->print(os); }

#if 0

struct SideCarMessage
{
    uint16_t magic_;		// 0xAAAA
    uint16_t alignment_;	// Zero is litle-endian
    uint32_t payloadSize_;	// Number of bytes in messsage
    uint16_t headerVersion_;	// Currently at 2
    uint16_t guidVersion_;	// Currently at 3
    uint32_t producerLength_;	// Number of characters in producer name
    char     producer_[producerLength_];

    // Pad to align on 2-byte boundary
    //
    uint16_t messageTypeKey_;

    // Pad to align to 4-byte boundary
    //
    uint32_t sequenceNumber_;
    int32_t  createdTimeStampSeconds_;
    int32_t  createdTimeStampMicroSeconds_;
    int32_t  emittedTimeStampSeconds_;
    int32_t  emittedTimeStampMicroSeconds_;

    uint16_t messageVersion_;	// Currently at 4

    // Pad to align to 4-byte boundary
    //
    uint32_t vmeMessageFlags_;
    uint32_t vmeTimeStamp_;
    uint32_t vmeSequence_;
    uint32_t vmeShaftEncoding_;
    uint32_t vmeNorthMark_;

    // Pad to align on 8-byte boundaries
    //
    double   vmeIRIGTime_;
    double   vmeRangeMin_;
    double   vmeRangeFactor_;

    uint32_t sampleCount_;
    int16_t  samples_[sampleCount_];
};

#endif

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
