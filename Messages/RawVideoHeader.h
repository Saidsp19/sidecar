#ifndef SBC_MSG_DEFS_H // -*- C++ -*-
#define SBC_MSG_DEFS_H

#include <stdint.h>

namespace SideCar {
namespace Messages {

#if 0

#define PORT_MASK 0x0000FFFF
#define MSG_FORMAT_MASK 0x00070000
#define IRIG_VALID_MASK 0x00080000
#define TIMESTAMP_VALID_MASK 0X00100000
#define AZ_VALID_MASK 0x00200000
#define PRI_VALID_MASK 0x00400000
/** \file
 */

#define ENDIANESS_MASK 0x00800000

// Message Formats
#define STATUS_MSG 0x00000
#define CONFIG_MSG 0x10000
#define PACKED_REAL_MSG 0x20000
#define PACKED_IQ_MSG 0x30000
#define UNPACKED_REAL_MSG 0x40000

// Endianess
/** \file
 */

#define BIG_ENDIAN 0x000000
/** \file
 */

#define LITTTLE_ENDIAN 0X800000;

/** \file
 */

#endif

struct SbcMsgHeader {
    enum MsgDescMasks {
        kPortMask = 0x0000FFFF,
        kFormatMask = 0x00070000,
        kIRIGValidMask = 0x00080000,
        kTimeStampValidMask = 0x00100000,
        kAzimuthValidMask = 0x00200000,
        kPRIValidMask = 0x00400000,
        kEndianessMask = 0x00800000,
    };

    enum Formats {
        kStatus = 0x00000,
        kConfiguration = 0x10000,
        kPackedReal = 0x20000,
        kPackedIQ = 0x30000,
        kUnpackedReal = 0x40000
    };

    uint32_t msgSize;   // Size of the data message
    uint32_t msgDesc;   // Describes the message
    uint32_t timeStamp; // Hires timestamp from TG card if available
    uint32_t azimuth;   // Azimuth from simulator or CRIB if available
    uint32_t pri;       // Pri counter, or message sequence counter
    uint32_t temp1;
    uint32_t temp2;
    uint32_t temp3;
    double irigTime; // IRIG or local time from IRIG card or simulator

    uint16_t getPort() const { return msgDesc & kPortMask; }
    uint16_t getFormat() const { return msgDesc & kFormatMask; }
    bool hasIRIG() const { return msgDesc & kIRIGValidMask; }
    bool hasTimeStamp() const { return msgDesc & kTimeStampValidMask; }
    bool hasAzimuth() const { return msgDesc & kAzimuthValidMask; }
    bool hasPRICounter() const { return msgDesc & kPRIValidMask; }
    std::string getFormattedIRIGTime();
};

struct SbcDataMsg {
    SbcMsgHeader header;
    double rangeMin;    // Range of first gate
    double rangeFactor; // Range change between gates
    uint32_t temp1;
    uint32_t temp2;
    uint32_t temp3;
    uint32_t numSamples;
    int16_t samples[1];
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
