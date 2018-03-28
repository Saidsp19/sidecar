#ifndef SIDECAR_IO_PROCESSINGSTATECHANGEREQUEST_H // -*- C++ -*-
#define SIDECAR_IO_PROCESSINGSTATECHANGEREQUEST_H

#include "IO/ControlMessage.h"
#include "IO/ProcessingState.h"

namespace SideCar {
namespace IO {

class ProcessingStateChangeRequest : public ControlMessage {
public:
    ProcessingStateChangeRequest(ProcessingState::Value state);

    ProcessingStateChangeRequest(ACE_Message_Block* data) : ControlMessage(data) {}

    ProcessingState::Value getValue() const;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
