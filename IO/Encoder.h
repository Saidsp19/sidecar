#ifndef SIDECAR_IO_ENCODER_H	// -*- C++ -*-
#define SIDECAR_IO_ENCODER_H

#include "ace/CDR_Stream.h"

#include "Messages/Header.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** Data encoder that makes CDR data from SideCar objects. An object that wishes to be encoded must provide a
    write() method that accepts an ACE_OutputCDR reference as its sole parameter and that returns the reference.
*/
class Encoder : public ACE_OutputCDR
{
public:

    /** Encode and append data from a given object. Relies on the object to do the encoding via its write()
     * method.

     \param message object to encode

     \return true if the encoding was successful
    */
    bool encode(const Messages::Header::Ref& message) { return (message->write(*this)).good_bit(); }
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
