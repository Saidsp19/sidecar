#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include "Logger/Log.h"

#include "ResolvedEntry.h"
#include "ServiceEntry.h"

using namespace SideCar::Zeroconf;

Logger::Log&
ServiceEntry::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Zeroconf.ServiceEntry");
    return log_;
}

ServiceEntry::Ref
ServiceEntry::Make(Monitor* monitor, const std::string& name, const std::string& type, const std::string& domain,
                   uint32_t interface)
{
    Ref ref(new ServiceEntry(monitor, name, type, domain, interface));
    ref->setSelf(ref);
    return ref;
}

ServiceEntry::ServiceEntry(Monitor* monitor, const std::string& name, const std::string& type,
                           const std::string& domain, uint32_t interface)
    : Transaction(monitor), name_(name), type_(type), domain_(domain), interface_(interface), resolved_(),
      resolvedSignal_()
{
    Logger::ProcLog log("ServiceEntry", Log());
    LOGINFO << name << ' ' << type << ' ' << domain << ' ' << interface << std::endl;
}

ServiceEntry::~ServiceEntry()
{
    ;
}

std::string
ServiceEntry::getInterfaceName() const
{
    char buffer[IF_NAMESIZE];
    return if_indextoname(interface_, buffer) ? buffer : "";
}

bool
ServiceEntry::resolve(bool blocking)
{
    Logger::ProcLog log("resolve", Log());
    LOGINFO << "blocking: " << blocking << " name: " << name_ << " type: " << type_ << std::endl;

    if (isRunning()) stop();
    resolved_.reset();
    auto err = ::DNSServiceResolve(&getDNSServiceRef(), 0, interface_, name_.c_str(), type_.c_str(),
                                   domain_.c_str(), &ServiceEntry::ResolveCallback, this);
    if (err != kDNSServiceErr_NoError) {
	LOGERROR << "failed DNSServiceResolve - error: " << err << std::endl;
	return false;
    }

    if (blocking) {
	DNSServiceProcessResult(getDNSServiceRef());
    }
    else {
	serviceStarted();
    }

    LOGINFO << "OK" << std::endl;
    return true;
}

void
ServiceEntry::ResolveCallback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface,
                              DNSServiceErrorType err, const char* fullName, const char* host, uint16_t port,
                              uint16_t textSize, const unsigned char* textData, void* context)
{
    Logger::ProcLog log("ResolveCallback", Log());
    port = ntohs(port);
    LOGINFO << err << ' ' << fullName << ' ' << host << ' ' << port << std::endl;
    reinterpret_cast<ServiceEntry*>(context)->processResponse(err, fullName, host, port, textSize, textData);
}

void
ServiceEntry::processResponse(DNSServiceErrorType err, const char* fullName, const char* host, uint16_t port,
                              uint16_t textSize, const unsigned char* textData)
{
    Logger::ProcLog log("processResponse", Log());
    LOGINFO << "err: " << err << std::endl;

    if (err != kDNSServiceErr_NoError) {
	LOGERROR << "failed DNSServiceResolve request - " << err << std::endl;
    }
    else {
	LOGDEBUG << "creating new ResolvedEntry object - " << host << '/' << port << std::endl;

	// Allocate ResolvedEntry object to hold the resolved data.
	//
	resolved_.reset(new ResolvedEntry(fullName, host, port, textSize, textData));

	// Notify anyone connected to our resolved signal.
	//
	resolvedSignal_(getSelf<ServiceEntry>());
    }

    // We are done with DNSServiceRef, close close it down.
    //
    finished();
}
