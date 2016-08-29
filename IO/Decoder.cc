#include "ace/ACE.h"
#include <errno.h>

#include "Logger/Log.h"

#include "Decoder.h"
#include "Preamble.h"

using namespace SideCar::IO;

Logger::Log&
Decoder::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Decoder");
    return log_;
}

// Acquire the data buffer used by an ACE_Message_Block, so that we don't make a copy of the encoded data.
//
Decoder::Decoder(ACE_Message_Block* data)
    : ACE_InputCDR(data->rd_ptr(), data->length()), data_(data), preamble_()
{
    static Logger::ProcLog log("Decoder", Log());
    LOGDEBUG << "rd_ptr: " << (data->rd_ptr() - data->base()) << " wr_ptr: " << (data->wr_ptr() - data->base())
	     << " length: " << data->length() << std::endl;

    LOGDEBUG << "source length: " << data->total_length() << " decoder length: " << length() << std::endl;

    assert(length() > 0);

    // Process the first eight bytes (2 32-bit integers) that make up the preamble to all messages. This has the
    // side-effect of changing the ACE_InputCDR byte order used for the incoming data.
    //
    preamble_.load(*this);
    LOGDEBUG << "swapping: " << (byte_order() != ACE_CDR_BYTE_ORDER) << " length: " << length() << std::endl;
    LOGDEBUG << "preamble.size: " << preamble_.getSize() << std::endl;
}

Decoder::~Decoder()
{
    data_->release();
}
