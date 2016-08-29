#ifndef SIDECAR_UTILS_IO_H // -*- C++ -*-
#define SIDECAR_UTILS_IO_H

#include <string>

class ACE_INET_Addr;
class ACE_InputCDR;
class ACE_OutputCDR;

/** Utility ACE CDR input stream extractor for C++ string objects. Reads in a 4 byte size followed by that
    number of characters.

    \param cdr stream to read from 

    \param string string to change

    \return stream read from
*/
extern bool
operator>>(ACE_InputCDR& cdr, std::string& string);

/** Utility ACE CDR output stream inserter for C++ string objects. Writes out a 4 byte size followed by the
    characters from the string.

    \param cdr stream to write to

    \param string string to write

    \return stream written to
*/
extern bool
operator<<(ACE_OutputCDR& cdr, const std::string& string);

namespace Utils {

extern std::string INETAddrToString(const ACE_INET_Addr& address);

}

/** \file
 */

#endif
