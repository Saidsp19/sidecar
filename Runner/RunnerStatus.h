#ifndef SIDECAR_RUNNERSTATUS_H // -*- C++ -*-
#define SIDECAR_RUNNERSTATUS_H

#include "IO/ProcessingState.h"
#include "IO/StreamStatus.h"

namespace Logger { class Msg; }

namespace SideCar {
namespace Configuration { class RunnerConfig; }
namespace Runner {

class RunnerStatus : public IO::StatusBase
{
public:

    enum {
	kConfigName = IO::StatusBase::kNumSlots,
	kServiceName,
	kHostName,
	kLogPath,
	kStreamStatus,
	kLogMessages,
	kMemoryUsed,
	kNumSlots
    };

    static const char* GetClassName() { return "RunnerStatus"; }

    static void Make(XmlRpc::XmlRpcValue& status, const Configuration::RunnerConfig& runnerConfig,
                     std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> streamStatus,
                     std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> logMessages,
                     double memoryUsed);

    RunnerStatus(const XmlRpc::XmlRpcValue& status)
	: IO::StatusBase(status) {}

    std::string getConfigName() const { return getSlot(kConfigName); }

    std::string getServiceName() const { return getSlot(kServiceName); }

    std::string getHostName() const { return getSlot(kHostName); }

    std::string getLogPath() const { return getSlot(kLogPath); }

    int getStreamCount() const { return getSlot(kStreamStatus).size(); }

    IO::StreamStatus getStreamStatus(int index) const
	{ return IO::StreamStatus(getSlot(kStreamStatus)[index]); }

    const XmlRpc::XmlRpcValue& getLogMessages() const
	{ return getSlot(kLogMessages); }

    double getMemoryUsed() const { return getSlot(kMemoryUsed); }
};

} // end namespace Runner
} // end namespace SideCar

/** \file
 */

#endif
