#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <time.h>

#include "VMEHeader.h"

using namespace SideCar::Messages;

ACE_InputCDR&
VMEHeader::load(ACE_InputCDR& cdr)
{
    cdr >> msgSize;
    cdr >> msgDesc;
    cdr >> timeStamp;
    cdr >> azimuth;
    cdr >> pri;
    cdr >> temp1;
    cdr >> temp2;
    cdr >> temp3;
    cdr >> irigTime;
    return cdr;
}

ACE_OutputCDR&
VMEHeader::write(ACE_OutputCDR& cdr) const
{
    cdr << msgSize;
    cdr << msgDesc;
    cdr << timeStamp;
    cdr << azimuth;
    cdr << pri;
    cdr << temp1;
    cdr << temp2;
    cdr << temp3;
    cdr << irigTime;
    return cdr;
}

std::ostream&
VMEHeader::print(std::ostream& os) const
{
    return os << "Size: " << msgSize << " Desc: " << std::hex << msgDesc << std::dec << ' '
              << ((msgDesc & kEndianessMask) ? 1 : 0) << ("SCRIU"[getFormat()]) << " 80Mhz: " << timeStamp
              << " AZ: " << azimuth << " PRI: " << pri << " IRIG: " << std::setprecision(16) << irigTime;
}

std::string
VMEHeader::getFormattedIRIGTime()
{
    time_t when = time_t(::floor(irigTime));
    struct tm* bits = ::gmtime(&when);
    std::string buffer(1024, ' ');
    strftime(const_cast<char*>(buffer.c_str()), 1024, "%c", bits);
    return buffer;
}

ACE_InputCDR&
VMEDataMessage::load(ACE_InputCDR& cdr)
{
    cdr >> header;
    cdr >> rangeMin;
    cdr >> rangeFactor;
    cdr >> temp1;
    cdr >> temp2;
    cdr >> temp3;
    cdr >> numSamples;
    return cdr;
}

ACE_OutputCDR&
VMEDataMessage::write(ACE_OutputCDR& cdr) const
{
    cdr << header;
    cdr << rangeMin;
    cdr << rangeFactor;
    cdr << temp1;
    cdr << temp2;
    cdr << temp3;
    cdr << numSamples;
    return cdr;
}

std::ostream&
VMEDataMessage::print(std::ostream& os) const
{
    return os << header << " Rmin: " << rangeMin << " Rf: " << rangeFactor << " samples: " << numSamples;
}
