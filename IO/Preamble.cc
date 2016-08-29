#include "IO/Preamble.h"
#include "Logger/Log.h"

using namespace SideCar::IO;

Logger::Log&
Preamble::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Preamble");
    return log_;
}

ACE_InputCDR&
Preamble::load(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("load", Log());

    // Fetch the byte order used by the sender, and adjust our input CDR accordingly.
    //
    cdr >> magicTag_;
    cdr >> byteOrder_;
    cdr.reset_byte_order(byteOrder_ ? 1 : 0);
    cdr >> size_;
    LOGDEBUG << "magicTag: " << std::hex << magicTag_ << std::dec << " byteOrder: " << byteOrder_
             << " size: " << size_ << std::endl;
    return cdr;
}

ACE_OutputCDR&
Preamble::write(ACE_OutputCDR& cdr) const
{
    // Write out byte order. Since the user cannot change this value, this is always ACE_CDR_BYTE_ORDER, which
    // reflects the endianness of the running system.
    //
    cdr << magicTag_;
    cdr << byteOrder_;
    cdr << size_;
    return cdr;
}
