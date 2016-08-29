#include <cstring>
#include <vector>

#include "MD5.h"

using namespace Utils;

/** Private utility class that implements the RSA MD5 algorithm.
 */
struct MD5::Private
{
    static auto F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    static auto G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    static auto H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    static auto I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

    static auto rotate(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

    static void FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
	{
	    a += F(b, c, d) + x + ac;
	    a = rotate(a, s);
	    a += b;
	}

    static void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
	{
	    a += G(b, c, d) + x + ac;
	    a = rotate(a, s);
	    a += b;
	}

    static void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
	{
	    a += H(b, c, d) + x + ac;
	    a = rotate(a, s);
	    a += b;
	}

    static void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
	{
	    a += I(b, c, d) + x + ac;
	    a = rotate(a, s);
	    a += b;
	}

    Private();

    void add(const uint8_t* ptr, size_t size);
    const DigestType& getDigest();

    void transform(const uint8_t* ptr);
    void encode(uint8_t* output, const uint32_t*, size_t);
    void copy(size_t, const uint8_t*, size_t);

    unsigned int state_[4];	 // State (ABCD)
    unsigned int count_[2];	 // Number of bits, modulo 2^64 (lsb first)
    unsigned char buffer_[kBufferSize]; // Input buffer
    DigestType digest_;		 // Digest result

    enum Constants {
	kS11 = 7,
	kS12 = 12,
	kS13 = 17,
	kS14 = 22,
	kS21 = 5,
	kS22 = 9,
	kS23 = 14,
	kS24 = 20,
	kS31 = 4,
	kS32 = 11,
	kS33 = 16,
	kS34 = 23,
	kS41 = 6,
	kS42 = 10,
	kS43 = 15,
	kS44 = 21
    };

    static const uint8_t kPadding_[kBufferSize];
};

const uint8_t MD5::Private::kPadding_[] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

MD5::Private::Private()
{
    count_[0] = count_[1] = 0;
    state_[0] = 0x67452301;
    state_[1] = 0xefcdab89;
    state_[2] = 0x98badcfe;
    state_[3] = 0x10325476;
}

inline void
MD5::Private::copy(size_t bufferIndex, const uint8_t* input, size_t count)
{
    uint8_t* p = &buffer_[bufferIndex];
    while (count--) {
	*p++ = *input++;
    }
}

void
MD5::Private::add(const uint8_t* input, size_t count)
{
    // Update the counter values.
    //
    uint32_t index  = (count_[0] >> 3) & 0x3F;
    if ((count_[0] += (count << 3)) < (count << 3)) {
	++count_[1];
    }
    count_[1] += count >> 29;

    uint32_t partLen = kBufferSize - index;
    uint32_t offset = 0;
    if (count >= partLen) {
	copy(index, input, partLen);
	transform(buffer_);
	for (offset = partLen; offset + 63 < count; offset += kBufferSize) {
	    transform(input + offset);
	}
	index = 0;
    }

    copy(index, input + offset, count - offset);
}

const MD5::DigestType&
MD5::Private::getDigest()
{
    if (count_[0] != 0 || count_[1] != 0) {

	// Calculate the digest value.
	//
	uint8_t bits[8];
	encode(bits, count_, 8);
	uint32_t index = (count_[0] >> 3) & 0x3f;
	uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
	add(kPadding_, padLen);
	add(bits, 8);
	encode(digest_, state_, kDigestSize);

	// Forget the state to help reduce the possibility that someone snooping our memory could reverse the
	// hashing.
	//
	::memset(state_, 0, sizeof(state_));
	::memset(buffer_, 0, sizeof(buffer_));

	// Zap the counters so that we won't recalculate the digest if asked for it again.
	//
	::memset(count_, 0, sizeof(count_));
    }

    return digest_;
}

void
MD5::Private::transform(const uint8_t* ptr)
{
    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t x[kDigestSize];

    // Initialize our hash array.
    //
    for (int i = 0, j = 0; j < kBufferSize; ++i, j += 4) {
	unsigned int t = static_cast<unsigned int>(*ptr++);
	t |= static_cast<unsigned int>(*ptr++) << 8;
	t |= static_cast<unsigned int>(*ptr++) << 16;
	t |= static_cast<unsigned int>(*ptr++) << 24;
	x[i] = t;
    }

    /* Round 1 */
    FF(a, b, c, d, x[0], kS11, 0xd76aa478); /* 1 */
    FF(d, a, b, c, x[1], kS12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[2], kS13, 0x242070db); /* 3 */
    FF(b, c, d, a, x[3], kS14, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[4], kS11, 0xf57c0faf); /* 5 */
    FF(d, a, b, c, x[5], kS12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[6], kS13, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[7], kS14, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[8], kS11, 0x698098d8); /* 9 */
    FF(d, a, b, c, x[9], kS12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], kS13, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], kS14, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], kS11, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], kS12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], kS13, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], kS14, 0x49b40821); /* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[1], kS21, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[6], kS22, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], kS23, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[0], kS24, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[5], kS21, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], kS22,  0x2441453); /* 22 */
    GG(c, d, a, b, x[15], kS23, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[4], kS24, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[9], kS21, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], kS22, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[3], kS23, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[8], kS24, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], kS21, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[2], kS22, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[7], kS23, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], kS24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[5], kS31, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[8], kS32, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], kS33, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], kS34, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[1], kS31, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[4], kS32, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[7], kS33, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], kS34, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], kS31, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[0], kS32, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[3], kS33, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[6], kS34,  0x4881d05); /* 44 */
    HH(a, b, c, d, x[9], kS31, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], kS32, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], kS33, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[2], kS34, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[0], kS41, 0xf4292244); /* 49 */
    II(d, a, b, c, x[7], kS42, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], kS43, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[5], kS44, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], kS41, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[3], kS42, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], kS43, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[1], kS44, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[8], kS41, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], kS42, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[6], kS43, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], kS44, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[4], kS41, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], kS42, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[2], kS43, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[9], kS44, 0xeb86d391); /* 64 */

    // Save our state.
    //
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;

    for (size_t index = 0; index < kDigestSize; ++index) x[index] = 0;
}

void
MD5::Private::encode(uint8_t* output, const uint32_t* input, size_t count)
{
    for (size_t i = 0, j = 0; j < count; ++i, j += 4) {
	output[j] = input[i] & 0xff;
	output[j + 1] = (input[i] >> 8) & 0xff;
	output[j + 2] = (input[i] >> 16) & 0xff;
	output[j + 3] = (input[i] >> 24) & 0xff;
    }
}

MD5::MD5()
    : p_(new MD5::Private)
{
    ;
}

// Needs to be defined here because 'p_' type is incomplete in header
//
MD5::~MD5()
{
    ;
}

void
MD5::add(const uint8_t* input, size_t count)
{
    p_->add(input, count);
}

const MD5::DigestType&
MD5::getDigest()
{
    return p_->getDigest();
}
