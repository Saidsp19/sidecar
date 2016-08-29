#include "RecordingStateChangeRequest.h"

using namespace SideCar::IO;

RecordingStateChangeRequest::RecordingStateChangeRequest(const std::string& path)
    : ControlMessage(kRecordingStateChange, path.size() + 1), path_(path)
{
    getData()->copy(path.c_str(), path.size() + 1);
}

RecordingStateChangeRequest::RecordingStateChangeRequest(ACE_Message_Block* data)
    : ControlMessage(data), path_(getData()->rd_ptr())
{
    ;
}
