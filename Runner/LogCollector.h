#ifndef SIDECAR_RUNNER_LOGCOLLECTOR_H // -*- C++ -*-
#define SIDECAR_RUNNER_LOGCOLLECTOR_H

#include <vector>

#include "Logger/Msg.h"
#include "Logger/Writers.h"
#include "Threading/Threading.h"
#include "XMLRPC/XmlRpcValue.h"

namespace SideCar {
namespace Runner {

class LogCollector : public Logger::Writers::Writer {
public:
    using Ref = boost::shared_ptr<LogCollector>;
    static Ref Make()
    {
        Ref ref(new LogCollector);
        return ref;
    }

    enum SlotIndex { kSeconds, kMicroSeconds, kChannel, kMessage, kPriorityLevel, kNumSlots };

    void write(const Logger::Msg& msg) override;

    std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> dump();

private:
    LogCollector();

    Threading::Mutex::Ref mutex_;
    std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> messages_;
};

} // end namespace Runner
} // end namespace SideCar

#endif
