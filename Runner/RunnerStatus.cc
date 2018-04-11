#include <iostream>
#include <unistd.h>

#include "Configuration/RunnerConfig.h"
#include "RunnerStatus.h"

using namespace SideCar::Configuration;
using namespace SideCar::Runner;

void
RunnerStatus::Make(XmlRpc::XmlRpcValue& status, const RunnerConfig& runnerConfig,
                   std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> streamStatus,
                   std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> logMessages, double memoryUsed)
{
    StatusBase::Make(status, kNumSlots, GetClassName(), runnerConfig.getRunnerName().toStdString());
    status[kConfigName] = runnerConfig.getConfigurationName().toStdString();
    status[kServiceName] = runnerConfig.getServiceName().toStdString();
    status[kHostName] = runnerConfig.getHostName().toStdString();
    status[kLogPath] = runnerConfig.getLogPath().toStdString();
    status[kStreamStatus] = streamStatus.release();
    status[kLogMessages] = logMessages.release();
    status[kMemoryUsed] = memoryUsed;
}
