#ifndef SIDECAR_MESSAGES_COMPLEX_H // -*- C++ -*-
#define SIDECAR_MESSAGES_COMPLEX_H

#include "Messages/RawVideo.h"

namespace SideCar {
namespace Messages {

/** Collection of gate values for one PRI message from a radar. The gate values are fixed-point, 16-bit values.
 */
class Complex : public TPRIMessage<Traits::ComplexInt16>
{
public:
    using Super = TPRIMessage<Traits::ComplexInt16>;
    using Ref = TPRIMessageRef<Complex>;

    /** Obtain the message type information for Complex objects.

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo& GetMetaTypeInfo();

    /** Factory method to create a new Complex message instance

        \param producer 

        \param vmeHeader 

        \param count 

        \return 
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, size_t count);

    /** 

        \param producer 

        \param vmeHeader 

        \param count 

        \return 
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, const DatumType* first,
                    const DatumType* end);

    /** 

        \param producer 

        \param vmeHeader 

        \param count 

        \return 
    */
    static Ref Make(const std::string& producer, const VMEDataMessage& vme, const Container& copy);

    /** Class factory used to create a Complex object from another Complex object.

	\param producer name of the entity that is creating the new object

        \param copy object to copy

        \return reference to new Complex object
    */
    static Ref Make(const std::string& producer, const PRIMessage::Ref& basis);

    /** Class factory used to create a new Complex object from an byte stream.

        \param cdr input stream to load from

        \return reference to new Complex object
    */
    static Ref Make(ACE_InputCDR& cdr);

    /** Obtain the Complex message that was the basis for this instance.

        \return Complex message or NULL if none exist
    */
    Ref getComplexBasis() const { return getBasis<Complex>(); }

private:

    Complex();

    Complex(const std::string& producer);

    Complex(const std::string& producer, const VMEDataMessage& vme, size_t size);

    Complex(const std::string& producer, const VMEDataMessage& vme, const Container& data);

    Complex(const std::string& producer, const PRIMessage::Ref& basis);

    /** Class factory used to create a new Complex object from an byte stream.

        \param cdr input stream to load from

        \return reference to new Complex object
    */
    static Header::Ref CDRLoader(ACE_InputCDR& cdr);

    /** Class factory used to create a new Complex object from an byte stream.

        \param cdr input stream to load from

        \return reference to new Complex object
    */
    static Header::Ref XMLLoader(const std::string& producer, XmlStreamReader& xsr);

    static MetaTypeInfo metaTypeInfo_;
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
