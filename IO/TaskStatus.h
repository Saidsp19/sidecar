#ifndef SIDECAR_IO_TASKSTATUS_H // -*- C++ -*-
#define SIDECAR_IO_TASKSTATUS_H

#include "IO/StatusBase.h"

namespace SideCar {
namespace IO {

class Stats;

/** Extension of StatusBase to include Task-specific information, such as processing state, seen message count,
    and mesage and byte rates.
*/
class TaskStatus : public StatusBase
{
public:

    enum {
	kProcessingState = StatusBase::kNumSlots,
	kError,
	kConnectionInfo,
	kMessageCount,
	kByteRate,
	kMessageRate,
	kDropCount,
	kDupeCount,
	kPendingQueueCount,
	kHasParameters,
	kUsingData,
	kNumSlots
    };

    static const char* GetClassName() { return "TaskStatus"; }

    TaskStatus(const XmlRpc::XmlRpcValue& status) : StatusBase(status) {}

    /** Obtain the processing state value held in the status container.

        \return ProcessingState::Value value
    */
    ProcessingState::Value getProcessingState() const
	{ return ProcessingState::Value(int(getSlot(kProcessingState))); }

    std::string getError() const { return getSlot(kError); }

    std::string getConnectionInfo() const { return getSlot(kConnectionInfo); }

    /** Obtain the running message count held in the status container.

        \return message count
    */
    int getMessageCount() const { return getSlot(kMessageCount); }

    int getByteRate() const { return getSlot(kByteRate); }

    int getMessageRate() const { return getSlot(kMessageRate); }

    int getDropCount() const { return getSlot(kDropCount); }

    int getDupeCount() const { return getSlot(kDupeCount); }

    int getPendingQueueCount() const { return getSlot(kPendingQueueCount); }

    bool hasParameters() const { return getSlot(kHasParameters); }

    bool isUsingData() const { return getSlot(kUsingData); }
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
