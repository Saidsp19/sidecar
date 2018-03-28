#include <iostream>

#include "LogCollector.h"

using namespace SideCar::Runner;

LogCollector::LogCollector() :
    Logger::Writers::Writer(0, false), mutex_(Threading::Mutex::Make()), messages_(new XmlRpc::XmlRpcValue::ValueArray)
{
    ;
}

void
LogCollector::write(const Logger::Msg& msg)
{
    XmlRpc::XmlRpcValue slot;
    slot.setSize(kNumSlots);
    slot[kSeconds] = int(msg.when_.tv_sec);
    slot[kMicroSeconds] = int(msg.when_.tv_usec);
    slot[kChannel] = msg.channel_;
    slot[kMessage] = msg.message_;
    slot[kPriorityLevel] = int(msg.level_);
    Threading::Locker locker(mutex_);
    messages_->push_back(slot);
}

std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray>
LogCollector::dump()
{
    static const size_t kMaxLogMessages = 200;

    std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> rc(new XmlRpc::XmlRpcValue::ValueArray);
    {
        Threading::Locker locker(mutex_);
        rc.swap(messages_);
    }

    // Limit the number of log messages we write out.
    //
    if (rc->size() > kMaxLogMessages) {
        size_t offset = rc->size() - kMaxLogMessages;
        for (size_t index = 0; index < kMaxLogMessages; ++index) { (*rc)[index] = (*rc)[index + offset]; }
        rc->resize(kMaxLogMessages);
    }

    return rc;
}
