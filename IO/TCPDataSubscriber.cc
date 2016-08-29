#include "Logger/Log.h"

#include "TCPConnector.h"
#include "TCPDataSubscriber.h"

using namespace SideCar;
using namespace SideCar::IO;

Logger::Log&
TCPDataSubscriber::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.IO.TCPDataSubscriber");
    return log_;
}

TCPDataSubscriber::Ref
TCPDataSubscriber::Make()
{
    Ref ref(new TCPDataSubscriber);
    return ref;
}

TCPDataSubscriber::TCPDataSubscriber()
    : Super(), connector_(0)
{
    static Logger::ProcLog log("TCPDataSubscriber", Log());
    LOGINFO << std::endl;
}

bool
TCPDataSubscriber::openAndInit(const std::string& key,
                               const std::string& serviceName,
                               int bufferSize, int interface)
{
    static Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " serviceName: " << serviceName
	    << " interface: " << interface << std::endl;

    setMetaTypeInfoKeyName(key);

    if (! reactor()) reactor(ACE_Reactor::instance());
    setError("Not connected to publisher");

    connector_ = new TCPConnector(this, bufferSize);
    LOGDEBUG << "connector: " << connector_ << std::endl;

    return Super::openAndInit(key, serviceName, MakeTwinZeroconfType(key),
                              interface);
}

int
TCPDataSubscriber::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << getTaskName() << " flags: " << flags << std::endl;
    if (connector_) {
	connector_->close();
	delete connector_;
	connector_ = 0;
    }

    return Super::close(flags);
}

void
TCPDataSubscriber::setServiceName(const std::string& serviceName)
{
    Super::setServiceName(serviceName);
    std::string taskName(serviceName);
    taskName += " SUB (TCP)";
    setTaskName(taskName);
}

void
TCPDataSubscriber::resolvedService(
    const Zeroconf::ServiceEntry::Ref& serviceEntry)
{
    static Logger::ProcLog log("resolvedService", Log());
    const Zeroconf::ResolvedEntry& resolved = serviceEntry->getResolvedEntry();
    LOGINFO << getTaskName() << " host: " << resolved.getHost() << '/'
	    << resolved.getPort() << std::endl;

    // Close down any existing connection.
    //
    connector_->close();

    std::string host(resolved.getHost());
    ACE_INET_Addr remoteAddress;
    remoteAddress.set(resolved.getPort(), resolved.getHost().c_str(), 1,
                      AF_INET);

    if (! connector_->openAndInit(remoteAddress, reactor())) {
	LOGERROR << getTaskName() << "failed to open TCPConnector with address "
		 << resolved.getHost() << '/' << resolved.getPort()
		 << std::endl;
	setError("Failed to initialize connector");
    }

    std::ostringstream os;
    os << "Host: " << host << " Port: " << resolved.getPort()
       << " Interface: " << serviceEntry->getInterfaceName();
    if (connector_->getMaxSocketBufferSize())
	os << " Buffer Size: " << connector_->getMaxSocketBufferSize();
    setConnectionInfo(os.str());
}

void
TCPDataSubscriber::lostService()
{
    static Logger::ProcLog log("lostService", Log());
    LOGINFO << getTaskName() << std::endl;
    connector_->close();
    setError("Not connected to publisher");
    setConnectionInfo("");
}
