#ifndef SIDECAR_ALGORITHMS_CONTROLLERSTATUS_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CONTROLLERSTATUS_H

#include "IO/TaskStatus.h"

namespace SideCar {
namespace Algorithms {

/** Status information reported by an algorithm controller. Contains recording status information, and custom
    info data and formatter specification obtained by the Algorithm::getInfoData() method.
*/
class ControllerStatus : public IO::TaskStatus
{
public:

    /** Indices of ControllerStatus values.
     */
    enum {
	kRecordingEnabled = IO::TaskStatus::kNumSlots,
	kRecordingOn,
	kRecordingQueueCount,
	kAlgorithmName,
	kAverageProcessingTime,
	kMinimumProcessingTime,
	kMaximumProcessingTime,
	kNumSlots
    };

    static const char* GetClassName() { return "ControllerStatus"; }

    ControllerStatus(const XmlRpc::XmlRpcValue& status)
	: IO::TaskStatus(status) {}

    bool isRecordingEnabled() const { return getSlot(kRecordingEnabled); }

    bool isRecordingOn() const { return getSlot(kRecordingOn); }

    int getRecordingQueueCount() const
	{ return getSlot(kRecordingQueueCount); }

    std::string getAlgorithmName() const { return getSlot(kAlgorithmName); }

    double getAverageProcessingTime() const
	{ return getSlot(kAverageProcessingTime); }
    double getMinimumProcessingTime() const
	{ return getSlot(kMinimumProcessingTime); }
    double getMaximumProcessingTime() const
	{ return getSlot(kMaximumProcessingTime); }
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
