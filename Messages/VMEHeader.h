#ifndef SIDECAR_MESSAGES_VMEHEADER_H // -*- C++ -*-
#define SIDECAR_MESSAGES_VMEHEADER_H

#include <stdint.h>

#include "IO/CDRStreamable.h"
#include "IO/Printable.h"

namespace SideCar {
namespace Messages {

struct VMEHeader : public IO::CDRStreamable<VMEHeader>, public IO::Printable<VMEHeader> {
    enum MsgDescMasks {
        kPortMask = 0x0000FFFF,
        kFormatMask = 0x00070000,
        kIRIGValidMask = 0x00080000,
        kTimeStampValidMask = 0x00100000,
        kAzimuthValidMask = 0x00200000,
        kPRIValidMask = 0x00400000,
        kEndianessMask = 0x00800000,
    };

    enum Formats { kStatus = 0, kConfiguration, kPackedReal, kPackedIQ, kUnpackedReal };

    uint16_t getPort() const { return msgDesc & kPortMask; }

    uint16_t getFormat() const { return (msgDesc & kFormatMask) >> 16; }

    bool hasIRIG() const { return msgDesc & kIRIGValidMask; }
    bool hasTimeStamp() const { return msgDesc & kTimeStampValidMask; }
    bool hasAzimuth() const { return msgDesc & kAzimuthValidMask; }
    bool hasPRICounter() const { return msgDesc & kPRIValidMask; }
    std::string getFormattedIRIGTime();

    ACE_InputCDR& load(ACE_InputCDR& cdr);
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;
    std::ostream& print(std::ostream& os) const;

    uint32_t msgSize;   // Size of the data message
    uint32_t msgDesc;   // Describes the message
    uint32_t timeStamp; // Hires timestamp from TG card if available
    uint32_t azimuth;   // Azimuth from simulator or CRIB if available
    uint32_t pri;       // Pri counter, or message sequence counter
    uint32_t temp1;
    uint32_t temp2;
    uint32_t temp3;
    double irigTime; // IRIG or local time from IRIG card or simulator
};

struct VMEDataMessage : public IO::CDRStreamable<VMEDataMessage>, public IO::Printable<VMEDataMessage> {
    ACE_InputCDR& load(ACE_InputCDR& cdr);
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;
    std::ostream& print(std::ostream& os) const;

    VMEHeader header;
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
