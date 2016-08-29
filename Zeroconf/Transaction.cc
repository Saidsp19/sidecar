#include "Logger/Log.h"
#include "Utils/Exception.h"

#include "Monitor.h"
#include "Transaction.h"

using namespace SideCar::Zeroconf;

Logger::Log&
Transaction::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Zeroconf.Transaction");
    return log_;
}

Transaction::Transaction(Monitor* monitor)
    : ref_(0), monitor_(monitor), self_(), finished_(false)
{
    monitor->setMonitored(this);
}

Transaction::~Transaction()
{
    Logger::ProcLog log("~Transaction", Log());
    LOGINFO << "ref: " << ref_ << " monitor: " << monitor_.get() << std::endl;
    stop();
}

bool
Transaction::stop()
{
    Logger::ProcLog log("stop", Log());
    LOGINFO << ref_ << std::endl;
    if (! isRunning()) return false;
    serviceStopping();
    DNSServiceRefDeallocate(ref_);
    ref_ = 0;
    return true;
}

void
Transaction::serviceStarted()
{
    Logger::ProcLog log("serviceStarted", Log());
    LOGINFO << ref_ << std::endl;
    monitor_->serviceStarted();
}

void
Transaction::serviceStopping()
{
    Logger::ProcLog log("serviceStopping", Log());
    LOGINFO << ref_ << std::endl;
    monitor_->serviceStopping();
}

bool
Transaction::processConnection()
{
    Logger::ProcLog log("processConnection", Log());
    LOGINFO << ref_ << std::endl;
    if (! isRunning()) return false;
    bool rc = DNSServiceProcessResult(ref_) == kDNSServiceErr_NoError;
    if (finished_) stop();
    return rc;
}
