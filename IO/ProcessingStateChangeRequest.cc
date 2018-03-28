#include "ProcessingStateChangeRequest.h"

using namespace SideCar::IO;

ProcessingStateChangeRequest::ProcessingStateChangeRequest(ProcessingState::Value state) :
    ControlMessage(kProcessingStateChange, sizeof(state))
{
    getData()->copy(reinterpret_cast<char*>(&state), sizeof(state));
}

ProcessingState::Value
ProcessingStateChangeRequest::getValue() const
{
    return *reinterpret_cast<ProcessingState::Value*>(getData()->rd_ptr());
}
