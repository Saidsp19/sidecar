#include "ace/CDR_Stream.h"
#include "ace/INET_Addr.h"

#include "IO.h"

#if 0

bool
operator>>(ACE_InputCDR& cdr, std::string& value)
{
    // Fetch the size, adjust the destination's size, and read in the data from the stream.
    //
    uint32_t size;
    cdr >> size;
    value.resize(size, ' ');

    // NOTE: no byte-swapping needed for single-character (ANSI) data. For multi-byte strings, use a different
    // approach.
    //
    cdr.read_char_array(const_cast<char*>(value.c_str()), size);
    return cdr.good_bit();
}

bool
operator<<(ACE_OutputCDR& cdr, const std::string& value)
{
    // Write out the size followed by the character data.
    //
    uint32_t size = value.size();
    cdr << size;
    cdr.write_char_array(value.c_str(), size);
    return cdr.good_bit();
}

#endif

std::string
Utils::INETAddrToString(const ACE_INET_Addr& address)
{
    ACE_TCHAR buffer[1024];
    int rc = address.addr_to_string(buffer, sizeof(buffer));
    if (rc == -1) buffer[0] = 0;
    return std::string(buffer);
}
