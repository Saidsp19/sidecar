#include "ace/Reactor.h"

#include "Logger/Log.h"
#include "Zeroconf/ACEMonitor.h"
#include "Zeroconf/Publisher.h"

#include "DataPublisher.h"

using namespace SideCar::IO;
using namespace SideCar::Zeroconf;

Logger::Log&
DataPublisher::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.DataPublisher");
    return log_;
}

DataPublisher::DataPublisher() :
    Super(), connectionPublisher_(Publisher::Make(new ACEMonitor)), serviceName_(""), timer_(-1)
{
    connection_ = connectionPublisher_->connectToPublishedSignal([this](auto v) { publishCallback(v); });
}

DataPublisher::~DataPublisher()
{
    connection_.disconnect();
    if (connectionPublisher_) {
        connectionPublisher_->stop();
        connectionPublisher_.reset();
    }
}

bool
DataPublisher::publish(const std::string& serviceName)
{
    static Logger::ProcLog log("publish", Log());
    LOGINFO << "serviceName: " << serviceName << std::endl;

    if (!reactor()) reactor(ACE_Reactor::instance());
    setServiceName(serviceName);

    if (timer_ == -1) {
        // Delay a random amount to stagger publishing requests.
        //
        const ACE_Time_Value delay(1 + ::drand48());
        const ACE_Time_Value repeatInterval(5);
        timer_ = reactor()->schedule_timer(this, &timer_, delay, repeatInterval);
        if (timer_ == -1) {
            LOGERROR << getTaskName() << " failed to schedule timer for "
                     << "reconnecting" << std::endl;
            setError("Failed to schedule publish attempt");
            return false;
        }
    }

    return true;
}

int
DataPublisher::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << "flags: " << flags << std::endl;

    if (flags) {
        if (timer_ != -1) {
            reactor()->cancel_timer(timer_);
            timer_ = -1;
        }

        if (connectionPublisher_) {
            connectionPublisher_->stop();
            connectionPublisher_.reset();
        }
    }

    return Super::close(flags);
}

void
DataPublisher::publishCallback(bool state)
{
    Logger::ProcLog log("publishCallback", Log());
    LOGINFO << getTaskName() << ' ' << state << std::endl;
    if (state) {
        publishSucceeded();
    } else {
        publishFailed();
    }
}

void
DataPublisher::setServiceName(const std::string& serviceName)
{
    serviceName_ = serviceName;
}

void
DataPublisher::publishSucceeded()
{
    Logger::ProcLog log("publishSucceeded", Log());
    LOGINFO << std::endl;
    if (connectionPublisher_->getName() != serviceName_) { setServiceName(connectionPublisher_->getName()); }
    if (timer_ != -1) {
        reactor()->cancel_timer(timer_);
        timer_ = -1;
    }
    enterLastProcessingState();
}

void
DataPublisher::publishFailed()
{
    Logger::ProcLog log("publishFailed", Log());
    LOGERROR << std::endl;

    setError("Name conflict");

    if (timer_ == -1) {
        const ACE_Time_Value delay(1);
        const ACE_Time_Value repeatInterval(5);
        timer_ = reactor()->schedule_timer(this, &timer_, delay, repeatInterval);
        if (timer_ == -1) {
            LOGERROR << getTaskName() << " failed to schedule timer for "
                     << "reconnecting" << std::endl;
            setError("Failed to schedule publish attempt");
        }
    }
}

int
DataPublisher::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << std::endl;

    if (arg != &timer_) { return 0; }

    if (!connectionPublisher_->publish(serviceName_, false)) {
        LOGERROR << "failed to publish connection with name '" << serviceName_
                 << " type: " << connectionPublisher_->getType() << std::endl;
        setError("Failed to publish connection info.");
    }

    return 0;
}
