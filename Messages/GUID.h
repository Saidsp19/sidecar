#ifndef SIDECAR_MESSAGES_GUID_H // -*- C++ -*-
#define SIDECAR_MESSAGES_GUID_H

#include "ace/Thread.h"

#include "IO/CDRStreamable.h"
#include "IO/Printable.h"
#include "Messages/LoaderRegistry.h"
#include "Messages/MetaTypeInfo.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Messages {

/** Definition of a globally-unique message ID. When a message is first created, it is assigned a GUID value
    that is unique for all hosts and applications running in the SideCar system.
*/
class GUID : public IO::CDRStreamable<GUID>, public IO::Printable<GUID>
{
public:

    static Logger::Log& Log();

    GUID();

    GUID(const std::string& producer, const MetaTypeInfo& typeInfo);

    GUID(const std::string& producer, const MetaTypeInfo& typeInfo, MetaTypeInfo::SequenceType sequenceNumber);

    ~GUID() {}

    const std::string& getProducerName() const { return producer_; }

    MetaTypeInfo::Value getMessageTypeKey() const { return messageTypeKey_; }

    MetaTypeInfo::SequenceType getMessageSequenceNumber() const { return messageSequenceNumber_; }

    void setMessageSequenceNumber(MetaTypeInfo::SequenceType seq_num) { messageSequenceNumber_ = seq_num; }

    const std::string& getSequenceKey() const;

    const std::string& getRepresentation() const;

    /** Obtain new instance data from a CDR input stream

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write out the header to a CDR output stream. NOTE: by design we only write out our time stamp -- the
        byte order and size are handled by one of the IO::Sink classes.

        \param cdr CDR stream to write to

        \return CDR stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    /** Write out the header values to a C++ text output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const;

private:

    GUID(const GUID& rhs);

    GUID& operator=(const GUID& rhs);

    std::string producer_;
    MetaTypeInfo::Value messageTypeKey_;
    MetaTypeInfo::SequenceType messageSequenceNumber_;
    mutable std::string sequenceKey_;
    mutable std::string representation_;

    static ACE_InputCDR& LoadV1(GUID* obj, ACE_InputCDR& cdr);
    static ACE_InputCDR& LoadV2(GUID* obj, ACE_InputCDR& cdr);
    static ACE_InputCDR& LoadV3(GUID* obj, ACE_InputCDR& cdr);
    static TLoaderRegistry<GUID>::VersionedLoaderVector DefineLoaders();
    static TLoaderRegistry<GUID> loaderRegistry_;
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
