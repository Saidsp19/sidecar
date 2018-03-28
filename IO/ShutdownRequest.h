#ifndef SIDECAR_IO_SHUTDOWNREQUEST_H // -*- C++ -*-
#define SIDECAR_IO_SHUTDOWNREQUEST_H

#include "IO/ControlMessage.h"

namespace SideCar {
namespace IO {

/** Control message that causes ShutdownMonitor to stop ACE all processing, effectively shutting down all
    streams, modules, and tasks.
*/
class ShutdownRequest : public ControlMessage {
public:
    ShutdownRequest() : ControlMessage(kShutdown, 0) {}
    ShutdownRequest(ACE_Message_Block* data) : ControlMessage(data) {}
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
