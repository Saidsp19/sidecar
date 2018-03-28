#ifndef SIDECAR_IO_DECODER_H // -*- C++ -*-
#define SIDECAR_IO_DECODER_H

#include "ace/CDR_Stream.h"
#include "boost/shared_ptr.hpp"

#include "IO/Preamble.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** Demarshalling class that converts raw bytes from readers into objects. Contains template methods which rely
    on object facilities to do the actual extraction from a CDR stream.
*/
class Decoder : public ACE_InputCDR {
public:
    /** Log device for Decoder objects

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor. Initializes an input CDR stream with data. Makes a "duplicate" of the ACE_Data_Block object
        held by the given ACE_Message_Block, which just increments the reference count in the ACE_Data_Block
        object. This results in sharing of the raw data between the given ACE_Message_Block and the ACE_InputCDR
        object. Extracts the first sizeof(Preamble) bytes into the preamble_ attribute.

        \param data container with the data to decode. NOTE: assumes ownership of the give message block. Use \c
        data->duplicate() to keep ownership of the message block.
    */
    Decoder(ACE_Message_Block* data);

    /** Destructor. Releases held data block.
     */
    ~Decoder();

    /** Type-specific extractor for objects. Relies on the object type to create itself using data found in a
        CDR stream. The template parameter must provide a Make factory method that takes a Decoder reference as
        its sole argument.

        \return reference to created message.
    */
    template <typename T>
    typename boost::shared_ptr<T> decode()
    {
        return T::Make(*this);
    }

    const Preamble& getPreamble() const { return preamble_; }

    bool isValid() const { return preamble_.isValid(); }

    /** Obtain the message size as found in the message Preamble header.

        \return message size
    */
    size_t getMessageSize() const { return preamble_.getSize(); }

private:
    ACE_Message_Block* data_; ///< Data block to decode
    Preamble preamble_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
