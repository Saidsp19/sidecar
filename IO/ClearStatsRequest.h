#ifndef SIDECAR_IO_CLEARSTATSREQUEST_H // -*- C++ -*-
#define SIDECAR_IO_CLEARSTATSREQUEST_H

#include "IO/ControlMessage.h"

namespace SideCar {
namespace IO {

/** Control message that causes IO::Task objects to clear their runtime statistics.
 */
class ClearStatsRequest : public ControlMessage {
public:
    ClearStatsRequest() : ControlMessage(kClearStats, 0) {}
    ClearStatsRequest(ACE_Message_Block* data) : ControlMessage(data) {}
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
