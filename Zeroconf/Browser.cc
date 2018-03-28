#include <sstream>

#include "Logger/Log.h"

#include "Browser.h"

using namespace SideCar::Zeroconf;

Logger::Log&
Browser::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Zeroconf.Browser");
    return log_;
}

Browser::Ref
Browser::Make(const MonitorFactory::Ref& monitorFactory, const std::string& type)
{
    Ref ref(new Browser(monitorFactory, type, "local.", 0));
    ref->setSelf(ref);
    return ref;
}

Browser::Browser(const MonitorFactory::Ref& monitorFactory, const std::string& type, const std::string& domain,
                 uint32_t interface) :
    Super(monitorFactory->make()),
    monitorFactory_(monitorFactory), type_(type), domain_(domain), interface_(interface), foundSignal_(), lostSignal_(),
    found_()
{
    Logger::ProcLog log("Browser", Log());
    LOGINFO << std::endl;
}

Browser::~Browser()
{
    stop();
}

bool
Browser::start()
{
    Logger::ProcLog log("start", Log());
    LOGINFO << "type: " << type_ << " domain: " << domain_ << " interface: " << interface_ << std::endl;

    // Start a browsing operation. We will receive notification of new services and lost services via the
    // processResponse callback.
    //
    if (isRunning()) stop();
    auto err = ::DNSServiceBrowse(&getDNSServiceRef(), 0, interface_, type_.c_str(), domain_.c_str(),
                                  &Browser::BrowseCallback, this);
    if (err != kDNSServiceErr_NoError) {
        LOGERROR << "failed DNSServiceBrowse - error: " << err << std::endl;
        return false;
    }

    // Notify the Transaction parent class that we've started an operation.
    //
    serviceStarted();

    return true;
}

void
Browser::BrowseCallback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface, DNSServiceErrorType err,
                        const char* name, const char* type, const char* domain, void* context)
{
    bool added = flags & kDNSServiceFlagsAdd;
    bool moreToCome = flags & kDNSServiceFlagsMoreComing;
    ServiceEntry::Ref entry;

    // Create a new ServiceEntry object to represent what was found.
    //
    auto browser = reinterpret_cast<Browser*>(context);
    browser->processResponse(err, name, type, domain, interface, added, moreToCome);
}

void
Browser::processResponse(DNSServiceErrorType err, const char* name, const char* type, const char* domain,
                         uint32_t interface, bool added, bool moreToCome)
{
    Logger::ProcLog log("processResponse", Log());
    LOGINFO << "err: " << err << " name: " << name << " type: " << type << " domain: "
            << " interface: " << interface << " added: " << added << " moreToCome: " << moreToCome << std::endl;
    if (err != kDNSServiceErr_NoError) {
        LOGERROR << "failed DNSServiceBrowse request - " << err << std::endl;
        return;
    }

    std::ostringstream key("");
    key << name << type << domain << interface;
    LOGDEBUG << "key: " << key.str() << std::endl;

    if (added) {
        ServiceEntry::Ref ref = ServiceEntry::Make(monitorFactory_->make(), name, type, domain, interface);
        found_[key.str()] = ref;
        finding_.push_back(ref);
        LOGDEBUG << "found " << ref->getName() << std::endl;
    } else {
        ServiceEntryMap::iterator pos = found_.find(key.str());
        if (pos != found_.end()) {
            ServiceEntry::Ref ref = pos->second;
            LOGDEBUG << "losing " << ref->getName() << std::endl;
            losing_.push_back(ref);
            found_.erase(pos);
        }
    }

    if (!moreToCome) {
        if (!losing_.empty()) {
            LOGDEBUG << "notifying about " << losing_.size() << " lost items" << std::endl;
            lostSignal_(losing_);
            losing_.clear();
        }

        if (!finding_.empty()) {
            LOGDEBUG << "notifying about " << finding_.size() << " new items" << std::endl;
            foundSignal_(finding_);
            finding_.clear();
        }
    }
}
