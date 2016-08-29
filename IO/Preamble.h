#ifndef SIDECAR_IO_PREAMBLE_H	// -*- C++ -*-
#define SIDECAR_IO_PREAMBLE_H

#include "ace/CDR_Stream.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** Definition of the preamble found at the beginning of all SideCar messages, regardless of content type. The
    preamble consists of 8 bytes (2 32-bit integers): the first indicates byte-ordering as defined by the ACE
    Common Data Representation (CDR) classes, and the second gives the number of bytes in the message body.

    Unlike the IP standard to always put things in network byte-order (big-endian), the CDR classes assume that
    the machine performing the writing (using ACE_OutputCDR) represents the optimal byte ordering. Thus, a
    program running on an Intel system (little-endian) that writes data to disk or network will do so with
    little-endian formatting (ie no byte-swapping). If the recipient of the message does not have the
    byte-ordering of the incoming message, ACE_InputCDR will perform the byte-swapping necessary to properly
    read the data of the message.
*/
class Preamble
{
public:

    /** Log device for Preamble objects
        
        \return log device
    */
    static Logger::Log& Log();

    enum {
	kCDRStreamSize = sizeof(int32_t) * 2, ///< Number of bytes in preamble
	kMagicTag = 0xAAAA                    ///< Magic value at start of msg
    };

    /** Constructor. Acquires the ACE CDR byte order for the running architecture. Initial size is set to zero.
	Used when writing new records to disk or network.
    */
    Preamble()
	: magicTag_(kMagicTag),
	  byteOrder_(ACE_CDR_BYTE_ORDER ? 0xFFFF : 0x0000), size_(0) {}

    /** Primary constructor. Acquires the ACE CDR byte order for the running architecture by default.

	\param size size to hold

	\param byteOrder little-endian (non-zero) or big-endian byte order
    */
    Preamble(uint32_t size, int byteOrder = ACE_CDR_BYTE_ORDER)
	: magicTag_(kMagicTag), byteOrder_(byteOrder ? 0xFFFF : 0x0000), size_(size) {}

    /** Conversion constructor for ACE_InputCDR objects. Reads in the magicTag_ and byteOrder_ attributes from
        the CDR object, and potentially changes the ACE_InputCDR to new byte ordering.

        \param cdr the ACE input CDR to use for data.
    */
    Preamble(ACE_InputCDR& cdr) { load(cdr); }

    /** Determine if the data seen so far matches a proper Preamble.

        \return true if so
    */
    bool isValid() const
	{ return magicTag_ == kMagicTag && (byteOrder_ == 0xFFFF || byteOrder_ == 0x0000); }

    /** Obtain the 'magic' tag of the preamble.

        \return initial 4-byte value of message
    */
    uint16_t getMagicTag() const { return magicTag_; }

    /** Obtain the byte ordering in use.

        \return ACE CDR byte order
    */
    uint16_t getByteOrder() const { return byteOrder_; }

    /** Obtain the size of the message body

        \return size in bytes
    */
    size_t getSize() const { return size_; }

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

private:
    uint16_t magicTag_;			///< 0xAAAA 0b1010101010101010
    uint16_t byteOrder_;		///< Byte order in effect
    uint32_t size_;			///< Size of message body in bytes
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
