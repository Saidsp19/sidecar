#ifndef SIDECAR_MESSAGES_BINARYVIDEO_H // -*- C++ -*-
#define SIDECAR_MESSAGES_BINARYVIDEO_H

#include "Messages/Video.h"

namespace SideCar {
namespace Messages {

/** Collection of gate values for one PRI message from a radar. The gate values are boolean, representing a
    true/false or pass/fail condition as the result of an algorithm.
*/
class BinaryVideo : public TPRIMessage<Traits::Bool>
{
public:
    using Super = TPRIMessage<Traits::Bool>;
    using Ref = TPRIMessageRef<BinaryVideo>;

    /** Obtain the message type information for BinaryVideo objets

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    static int GetAliveCount();

    /** Factory method that generates a new, empty BinaryVideo message. Reserves (allocates) space for \a count
        values, but the message is initially empty.

        \param producer the name of the task that created the message

        \param vmeHeader the VME parameters that define the message

        \param count the number of values to reserve

        \return new Video message reference
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, size_t count);

    /** Class factory method used to create a BinaryVideo object from a Video object.

	\param producer name of the entity that is creating the new object

        \param video video object to use

        \return reference to new BinaryVideo object
    */
    static Ref Make(const std::string& producer, const Video::Ref& video);

    /** Class factory method used to create a BinaryVideo object from another BinaryVideo object.

	\param producer name of the entity that is creating the new object

        \param video binary video object to use

        \return reference to new BinaryVideo object
    */
    static Ref Make(const std::string& producer, const BinaryVideo::Ref& video);

    /** 

        \param producer 

        \param vmeHeader 

        \param count 

        \return 
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, const DatumType* first,
                    const DatumType* end);

    /** Class factory used to create a new Video object from a byte stream.

        \param cdr input stream to load from

        \return reference to new Video object
    */
    static Ref Make(ACE_InputCDR& cdr);

    ~BinaryVideo();

    /** Obtain the first Video message that forms the basis for this instance.

        \return Video message found, or NULL if none exist
    */
    Video::Ref getVideoBasis() const;

    /** Obtain the BinaryVideo message that was the basis for this instance.

        \return BinaryVideo message or NULL if none exist
    */
    Ref getBinaryBasis() const { return getBasis<BinaryVideo>(); }

private:

    BinaryVideo();

    BinaryVideo(const std::string& producer, const VMEDataMessage& vme, size_t size);

    BinaryVideo(const std::string& producer, const Video::Ref& basis);

    BinaryVideo(const std::string& producer, const BinaryVideo::Ref& basis);

    BinaryVideo(const std::string& producer);

    /** Load procedure used to create a new BinaryVideo object using data from a raw data stream.

        \param cdr streamn containing the raw data

        \return reference to new BinaryVideo object
    */
    static Header::Ref CDRLoader(ACE_InputCDR& cdr);

    /** Load procedure used to create a new BinaryVideo object using data from a raw data stream.

        \param cdr streamn containing the raw data

        \return reference to new BinaryVideo object
    */
    static Header::Ref XMLLoader(const std::string& producer,
                                 XmlStreamReader& xsr);

    static MetaTypeInfo metaTypeInfo_;
};

inline std::ostream&
operator<<(std::ostream& os, const BinaryVideo::Ref& ref)
{ return ref->print(os); }

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
