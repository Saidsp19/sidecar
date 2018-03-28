#ifndef UTILS_MD5_H // -*- C++ -*-
#define UTILS_MD5_H

#include <inttypes.h> // Contains uint8_t
#include <string>
#include <vector>

namespace Utils {

/** MD5 hash generator. Accumulates data using its add() methods, and returns the hash of the accumulated data
    via its getDigest() method. NOTE: not to be used for cryptographic purposes.
*/
class MD5 {
public:
    enum { kDigestSize = 16, kBufferSize = 4 * kDigestSize };

    /** Container type for all MD5 hashes: 16 byte array.
     */
    using DigestType = uint8_t[kDigestSize];

    /** Constructor. Initialize generator constants
     */
    MD5();

    /** Destructor. Releases private memory.
     */
    ~MD5();

    /** Add bytes from an STL string to the hash function.

        \param buffer the data to add
    */
    void add(const std::string& buffer) { add(reinterpret_cast<const uint8_t*>(buffer.c_str()), buffer.size()); }

    /** Add bytes from an STL vector to the hash function.

        \param buffer the data to add
    */
    void add(const std::vector<uint8_t>& buffer) { add(&buffer[0], buffer.size()); }

    /** Add bytes from user-defined structure.

        \param buffer pointer to the first byte to add

        \param count the number of bytes to add
    */
    void add(const uint8_t* buffer, size_t count);

    /** Obtain the hash digest for the accumulated data

        \return read-only reference to a 16-byte array holding the digest
        values.
    */
    const DigestType& getDigest();

private:
    struct Private;
    std::unique_ptr<Private> p_;
};

} // end namespace Utils

#endif
