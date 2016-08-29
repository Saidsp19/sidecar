#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "Logger/Log.h"

#include "MessageManager.h"
#include "UDPSocketWriterTask.h"

using namespace SideCar::IO;

Logger::Log&
UDPSocketWriterTask::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.IO.UDPSocketWriterTask");
    return log_;
}

UDPSocketWriterTask::Ref
UDPSocketWriterTask::Make()
{
    Ref ref(new UDPSocketWriterTask);
    return ref;
}

UDPSocketWriterTask::UDPSocketWriterTask()
    : IOTask(), writer_()
{
    ;
}

bool
UDPSocketWriterTask::openAndInit(const std::string& key,
                                 const std::string& host, uint16_t port)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << key << ' ' << host << ' ' << port << std::endl;

    std::ostringstream os;
    os << "Host: " << host << " Port: " << port;
    setConnectionInfo(os.str());

    if (! getTaskName().size()) {
	std::ostringstream os;
	os << "UDPOut(" << key << ',' << host << ',' << port << ')';
	setTaskName(os.str());
    }

    setMetaTypeInfoKeyName(key);

    if (! reactor()) reactor(ACE_Reactor::instance());

    // Working directly with the ACE datagram (UDP) object.
    //
    ACE_SOCK_Dgram& device(writer_.getDevice());

    // Open a connection, obtaining a system-provided port number to use.
    //
    ACE_INET_Addr address(u_short(0), "0.0.0.0", AF_INET);
    LOGDEBUG << "opening UDP socket with address "
	     << Utils::INETAddrToString(address) << std::endl;
    if (device.open(address, AF_INET, 0, 1) == -1) {
	LOGERROR << "failed to open writer" << std::endl;
	return false;
    }

    // But we send out data to a specific host/port combination.
    //
    ACE_INET_Addr remoteAddress(port, host.c_str(), PF_INET);
    writer_.setRemoteAddress(remoteAddress);

    return true;
}

bool
UDPSocketWriterTask::deliverDataMessage(ACE_Message_Block* data,
                                        ACE_Time_Value* timeout)
{
    static Logger::ProcLog log("deliverDataMessage", Log());

    MessageManager mgr(data->duplicate());
    if (! writer_.write(mgr)) {
	LOGERROR << "failed to send the message" << std::endl;
	return false;
    }

    return put_next(data, timeout) != -1;
}
