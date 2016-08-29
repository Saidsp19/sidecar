#ifndef SIDECAR_MESSAGES_VIDEO_H // -*- C++ -*-
#define SIDECAR_MESSAGES_VIDEO_H

#include "Messages/PRIMessage.h"

namespace SideCar {
namespace Messages {

/** Collection of sample values for one PRI message from a radar. The sample values are fixed-point, 16-bit
    values (normally treated as integers).
*/
class Video : public TPRIMessage<Traits::Int16>
{
public:
    using Super = TPRIMessage<Traits::Int16>;
    using Ref = TPRIMessageRef<Video>;

    /** Obtain the message type information for Video objects.

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    /** Factory method that generates a new, empty Video message. Reserves (allocates) space for \a count
        values, but the message is initially empty.

        \param producer the name of the task that created the message

        \param vmeHeader the VME parameters that define the message

        \param count the number of values to reserve

        \return new Video message reference
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, size_t count);

    /** Factory method that generates a new Video message filled with values taken from a pointer.

        \param producer the name of the task that created the message

        \param vmeHeader the VME parameters that define the message

	\param first pointer to the first value to use

	\param end pointer to the last+1 value to use

        \return new Video message reference
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, const DatumType* first,
                    const DatumType* end);

    /** Factory method that generates a new Video message filled with values taken from a std::vector of values.

        \param producer the name of the task that created the message

        \param vmeHeader the VME parameters that define the message

        \param data container of values to copy

        \return 
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, const Container& data);

    /** Class factory used to create a Video object from another Video object.

	\param producer name of the entity that is creating the new object

        \param basis message that forms the basis for the new one

        \return reference to new Video object
    */
    static Ref Make(const std::string& producer, const PRIMessage::Ref& basis);

    /** Class factory used to create a new Video object from an byte stream.

        \param cdr input stream to load from

        \return reference to new Video object
    */
    static Ref Make(ACE_InputCDR& cdr);

    ~Video();

    /** Obtain the Video message that was the basis for this instance.

        \return Video message or NULL if none exist
    */
    Video::Ref getVideoBasis() const { return getBasis<Video>(); }

private:

    /** Constructor for empty messages.

        \param producer the name of the task that created the message

        \param vmeHeader the VME parameters that define the message

        \param count the number of values to reserve
    */
    Video(const std::string& producer, const VMEDataMessage& vme, size_t size);

    /** Constructor for message with values copied from a std::vector.

        \param producer the name of the task that created the message

        \param vmeHeader the VME parameters that define the message

        \param data container holding the values to copy
    */
    Video(const std::string& producer, const VMEDataMessage& vme, const Container& data);

    /** Constructor for messages that will be created from a CDR input stream.

        \param producer the name of the task that created the message
    */
    Video(const std::string& producer);

    /** Constructor for messages that are derived from another one.

        \param producer the name of the task that created the message

        \param basis the message we are based on
    */
    Video(const std::string& producer, const PRIMessage::Ref& basis);

    /** Default constructor.
     */
    Video();

    /** Class factory used to create a new Video object from an byte stream.

        \param cdr input stream to load from

        \return reference to new Video object
    */
    static Header::Ref CDRLoader(ACE_InputCDR& cdr);

    /** Class factory used to create a new Video object from an XML stream.

        \param cdr input stream to load from

        \return reference to new Video object
    */
    static Header::Ref XMLLoader(const std::string& producer, XmlStreamReader& xsr);

    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
