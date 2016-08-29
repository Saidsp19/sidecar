#ifndef SIDECAR_IO_STREAMSTATUS_H // -*- C++ -*-
#define SIDECAR_IO_STREAMSTATUS_H

#include "IO/StatusBase.h"
#include "IO/TaskStatus.h"

namespace SideCar {
namespace IO {

class StreamStatus : public StatusBase
{
public:

    enum {
	kTaskStatus = StatusBase::kNumSlots,
	kNumSlots
    };

    static const char* GetClassName() { return "StreamStatus"; }

    StreamStatus(const std::string& name);

    StreamStatus(const XmlRpc::XmlRpcValue& status) : StatusBase(status) {}

    int getTaskCount() const { return getSlot(kTaskStatus).size(); }

    TaskStatus getTaskStatus(int index) const
	{ return TaskStatus(getSlot(kTaskStatus)[index]); }
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
