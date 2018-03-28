#include <cstdlib>

#include "ace/Reactor.h"

#include "Logger/Log.h"
#include "Zeroconf/ACEMonitor.h"

#include "DataSubscriber.h"

using namespace SideCar;
using namespace SideCar::IO;

Logger::Log&
DataSubscriber::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.DataSubscriber");
    return log_;
}

DataSubscriber::DataSubscriber() :
    Super(), browser_(), serviceName_(""), resolvedSignalConnection_(), service_(), timerId_(-1)
{
    static Logger::ProcLog log("DataSubscriber", Log());
    LOGDEBUG << std::endl;
}

bool
DataSubscriber::openAndInit(const std::string& key, const std::string& serviceName, const std::string& type,
                            int interface)
{
    static Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " serviceName: " << serviceName << " type: " << type
            << " interface: " << interface << std::endl;

    setServiceName(serviceName);
    setMetaTypeInfoKeyName(key);

    // Create our publisher browser.
    //
    Zeroconf::ACEMonitorFactory::Ref monitorFactory(Zeroconf::ACEMonitorFactory::Make());
    browser_ = Zeroconf::Browser::Make(monitorFactory, type);

    browser_->connectToFoundSignal([this](auto& v) { foundNotification(v); });
    browser_->connectToLostSignal([this](auto& v) { lostNotification(v); });

    if (interface) { browser_->setInterface(interface); }

    // Create a delay of 0 - 3 seconds before browsing for publishers.
    //
    ACE_Time_Value delay;
    delay.set(3.0 * ::drand48());
    const ACE_Time_Value repeat(0);
    LOGWARNING << "delay: " << delay.sec() << ' ' << delay.usec() << std::endl;

    timerId_ = reactor()->schedule_timer(this, &timerId_, delay, repeat);
    setError("Preparing to browse...");

    return true;
}

void
DataSubscriber::restartBrowser()
{
    Logger::ProcLog log("restartBrowser", Log());
    LOGWARNING << std::endl;
    if (browser_) {
        service_.reset();
        browser_->start();
    }
}

int
DataSubscriber::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << std::endl;

    if (arg != &timerId_ || !browser_) {
        LOGERROR << "unknown timerId: " << arg << std::endl;
        return -1;
    }

    // Make sure we don't fire again
    //
    reactor()->cancel_timer(timerId_);
    timerId_ = -1;

    if (!browser_->start()) {
        LOGERROR << "failed to start Zeroconf browser" << std::endl;
        setError("Failed to start publisher browser.");
        return -1;
    }

    LOGINFO << "started Zeroconf browser" << std::endl;
    return 0;
}

int
DataSubscriber::close(u_long flags)
{
    if (flags) {
        if (service_) {
            resolvedSignalConnection_.disconnect();
            service_.reset();
        }

        if (timerId_ != -1) {
            reactor()->cancel_timer(timerId_);
            timerId_ = -1;
        }

        if (browser_) {
            browser_->stop();
            browser_.reset();
        }
    }

    return Super::close(flags);
}

void
DataSubscriber::foundNotification(const ServiceEntryVector& services)
{
    static Logger::ProcLog log("foundNotification", Log());
    LOGINFO << "size: " << services.size() << std::endl;

    Zeroconf::ServiceEntry::Ref best = service_;
    for (size_t index = 0; index < services.size(); ++index) {
        Zeroconf::ServiceEntry::Ref service(services[index]);
        LOGDEBUG << "looking for: " << serviceName_ << " found: " << service->getName()
                 << " interface: " << service->getInterface() << std::endl;
        if (service->getName() == serviceName_ && !best) { best = service; }
    }

    if (best && best != service_) {
        if (service_) { resolvedSignalConnection_.disconnect(); }

        service_ = best;
        LOGDEBUG << "best interface: " << best->getInterfaceName() << std::endl;

        resolvedSignalConnection_ = best->connectToResolvedSignal([this](auto& v) { resolvedNotification(v); });

        setError("Publisher found.", true);
        best->resolve();
    }
}

void
DataSubscriber::lostNotification(const ServiceEntryVector& services)
{
    static Logger::ProcLog log("lostNotification", Log());
    LOGINFO << "size: " << services.size() << std::endl;

    for (size_t index = 0; index < services.size(); ++index) {
        Zeroconf::ServiceEntry::Ref service(services[index]);
        LOGDEBUG << serviceName_ << ' ' << service->getName() << std::endl;
        if (service == service_) {
            LOGDEBUG << "lost " << serviceName_ << std::endl;
            service_.reset();
            resolvedSignalConnection_.disconnect();
            lostService();
            setError("Not connected to publisher.", true);
            return;
        }
    }
}

void
DataSubscriber::resolvedNotification(const Zeroconf::ServiceEntry::Ref& serviceEntry)
{
    static Logger::ProcLog log("resolvedNotification", Log());
    const Zeroconf::ResolvedEntry& resolved = serviceEntry->getResolvedEntry();
    LOGINFO << "getHost: " << resolved.getHost() << " getPort: " << resolved.getPort() << std::endl;
    resolvedService(serviceEntry);
}

bool
DataSubscriber::doProcessingStateChange(const ProcessingStateChangeRequest& msg)
{
    bool rc = Super::doProcessingStateChange(msg);
    if (rc && !service_) setError("Not connected to publisher.");
    return rc;
}

void
DataSubscriber::setServiceName(const std::string& serviceName)
{
    serviceName_ = serviceName;
}
