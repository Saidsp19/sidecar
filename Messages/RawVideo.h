#ifndef SIDECAR_MESSAGES_RAWVIDEO_H // -*- C++ -*-
#define SIDECAR_MESSAGES_RAWVIDEO_H

#include "Messages/PRIMessage.h"
#include "Messages/VMEHeader.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Messages {

class Video;

class RawVideo : public Header {
public:
    using Super = Header;
    using Ref = boost::shared_ptr<RawVideo>;

    static Logger::Log& Log();

    /** Obtain the message type information for RawVideo objects.

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    /** Class factory that creates new reference-counted RawVideo message objects

        \param producer name of the entity that is creating the new object

        \param raw data block containing the raw data from a VME system

        \return reference to new RawVideo object
    */
    static Ref Make(const std::string& producer, ACE_Message_Block* raw);

    /** Class factory that creates new reference-counted RawVideo message objects using data from an input CDR
        stream.

        \param cdr input CDR stream to read from

        \return reference to new RawVideo object
    */
    static Ref Make(ACE_InputCDR& cdr);

    /** Destructor.
     */
    ~RawVideo();

    size_t getSize() const { return raw_ ? raw_->length() : 0; }

    /** Conversion from RawVideo to Video format. Performs byte-swapping from VME (PowerPC) system to Intel
        system.

        \param producer name of the entity doing the conversion

        \return reference to new Video object
    */
    TPRIMessageRef<Video> convert(const std::string& producer) const;

    /** Read in raw VME data from an CDR input stream. Override of Header::load().

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write out raw VME data to a CDR output stream. Override of Header::write().

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

    /** Print out a textual representation of the raw VME data. Override of Header::print().

        \param os C++ text stream to write to

        \return stream written to
    */
    std::ostream& printData(std::ostream& os) const;

private:
    /** Constructor for new RawVideo message.

        \param producer name of the entity that created the message

        \param raw data block containing raw VME data.
    */
    RawVideo(const std::string& producer, ACE_Message_Block* raw, MetaTypeInfo::SequenceType sequenceNumber) :
        Super(producer, GetMetaTypeInfo(), Ref(), sequenceNumber), raw_(raw)
    {
    }

    /** Constructor for RawVideo messages that will be filled in with data from a CDR stream.
     */
    RawVideo() : Super(GetMetaTypeInfo()), raw_(0) {}

    ACE_Message_Block* raw_;

    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
