#ifndef BYTEORDER_H // -*- C++ -*-
#define BYTEORDER_H

#include <algorithm>
#include <sys/param.h>

/** Class to determine and support the byte-order on a particular platform. There are only two byte-orders
    currently supported: big-endian and little-endian. All of the RWSL code base should read and write data in
    big-endian mode, which is the same as network byte-order. Its nice that the Sun SPARC and Motorola/IBM
    PowerPC both operate in this mode, since those are the most common platforms we run on. However, if given an
    Intel, we should be able to run correctly on it.

    NOTE: Only used by SideCar::GUI::PRIEmittter. Everything else should use the IO::Encoder and IO::Decoder
    classes, which use the ACE CDR streaming classes.
*/
class ByteOrder {
public:
    enum Type {
        kLittleEndian = 0,
        kBigEndian,
        kNetwork = kBigEndian,

#if defined(sparc) || defined(__sparc) || defined(__ppc__)
        kHost = kBigEndian,
        kHostOpp = kLittleEndian
    /** \file
     */

#endif
#if defined(i386) || defined(__i386) || defined(__x86_64)
            kHost = kLittleEndian,
        kHostOpp = kBigEndian
    /** \file
     */

#endif
    };

    /** Determine if the host we are running on has network (big-endian) byte-order.

        \return true if so
    */
    static bool HostIsNetwork() { return kHost == kNetwork; }

    /** Constructor. Determine if we are little-endian and need to swap bytes.
     */
    ByteOrder() : decodeSwap_(!HostIsNetwork()), encodeSwap_(!HostIsNetwork()) {}

    /** Set the byte-order of data being read. If the value is NOT the same as the host value, then we will
        byte-swap incoming data.

        \param value new byte order setting.
    */
    void setDecodingByteOrder(Type value) { decodeSwap_ = value != kHost; }

    /** Set the byte-order of data being written. If the value is NOT the same as the host value, the we will
        byte-swap outgoing data.

        \param value new byte order setting
    */
    void setEncodingByteOrder(Type value) { encodeSwap_ = value != kHost; }

    /** Get the byte-order of data being read.

        \return byte order
    */
    Type getDecodingByteOrder() const { return decodeSwap_ ? kHostOpp : kHost; }

    /** Get the byte-order of data being written.

        \return byte order
    */
    Type getEncodingByteOrder() const { return encodeSwap_ ? kHostOpp : kHost; }

    /** Convert <b>in-place</b> a value that was read in from an external source.

        \param value read-write reference to value to convert
        `    */
    template <typename T>
    void decode(T& value) const
    {
        decode(&value, sizeof(T));
    }

    /** Convert <b>in-place</b> a value before it is written out to an external sink.

        \param value read-write reference to value to convert
    */
    template <typename T>
    void encode(T& value) const
    {
        encode(&value, sizeof(T));
    }

    /** Convert <b>in-place</b> a consecutive sequence of bytes that was read in from an external source.

        \param ptr start of the sequence

        \param size number of bytes in the sequence
    */
    void decode(void* ptr, size_t size) const
    {
        if (decodeSwap_) Reverse(ptr, size);
    }

    /** Convert <b>in-place</b> a consecutive sequence of bytes before it is written out to a sink.

        \param ptr start of the sequence

        \param size number of bytes in the sequence
    */
    void encode(void* ptr, size_t size) const
    {
        if (encodeSwap_) Reverse(ptr, size);
    }

private:
    /** Reverse a sequence of bytes. Uses ReverseLoop to do the work.

        \param ptr start of the sequence

        \param size number of bytes to process
    */
    static void Reverse(void* ptr, size_t size)
    {
        ReverseLoop(reinterpret_cast<unsigned char*>(ptr), reinterpret_cast<unsigned char*>(ptr) + size);
    }

    /** Reverse a sequence of bytes. Swaps the first and last bytes of a range, and then continues, swapping the
        second and second-to-last, until the first half of the range has been swapped with the second half.

        \param from pointer to first byte to swap

        \param to pointer to last byte + 1 to swap
    */
    static void ReverseLoop(unsigned char* from, unsigned char* to)
    {
        while (from < to) std::swap(*from++, *--to);
    }

    bool decodeSwap_; ///< True if swapping is necessary in input
    bool encodeSwap_; ///< True if swapping is necessary on output
};

/** \file
 */

#endif
