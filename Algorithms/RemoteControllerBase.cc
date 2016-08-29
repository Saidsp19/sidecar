#include <netinet/in.h>
#include <stdlib.h>
#include <string>

#include "ace/Reactor.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Stream.h"

#include "Logger/Log.h"
#include "XMLRPC/XmlRpcServer.h"
#include "Zeroconf/ACEMonitor.h"
#include "Zeroconf/Publisher.h"

#include "RemoteControllerBase.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Zeroconf;

Logger::Log&
RemoteControllerBase::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Algorithms.RemoteControllerBase");
    return log_;
}

RemoteControllerBase::RemoteControllerBase(const char* zeroconfType)
    : Super(), zeroconfType_(zeroconfType), rpcServer_(), connectionPublisher_(), running_(0)
{
    Logger::ProcLog log("RemoteControllerBase", Log());
    LOGINFO << std::endl;
}

RemoteControllerBase::~RemoteControllerBase()
{
    Logger::ProcLog log("~RemoteControllerBase", Log());
    LOGINFO << std::endl;
}

int
RemoteControllerBase::close(u_long flags)
{
    Logger::ProcLog log("svc", Log());
    LOGINFO << flags << std::endl;
    if (flags && running_) {
	if (! stop()) {
	    LOGERROR << "failed to stop controller" << std::endl;
	    return -1;
	}
    }

    return 0;
}

bool
RemoteControllerBase::start(const std::string& serviceName, long threadFlags,
                            long priority)
{
    Logger::ProcLog log("start", Log());
    LOGINFO << std::endl;

    serviceName_ = serviceName;

    if (running_) {
	LOGERROR << "already running" << std::endl;
	return false;
    }

    // The task requires a valid ACE_Reactor for the Zeroconf::ACEMonitor to work.
    //
    if (! reactor()) reactor(ACE_Reactor::instance());

    // Create and initialize a new XML-RPC server.
    //
    rpcServer_.reset(new XmlRpc::XmlRpcServer);
    installServices(rpcServer_.get());

    // Enable XML-RPC connections. Let system pick a free port to use.
    //
    if (! rpcServer_->bindAndListen(0)) {
	LOGERROR << "failed to create XMLRPC server socket" << std::endl;
	return false;
    }

    // Get the adddress of the XML-RPC server.
    //
    ACE_SOCK_Stream sock(rpcServer_->getfd());
    if (sock.get_local_addr(address_) == -1) {
	LOGERROR << "failed to get port for XMLRPC server" << std::endl;
	return false;
    }

    uint16_t port = address_.get_port_number();
    LOGINFO << "XMLRPC socket is running on port " << port << std::endl;

    // Create a new Zeroconf::Publisher to announce the connection information for the XML-RPC server.
    //
    std::string serviceType(zeroconfType_);
    connectionPublisher_ = Publisher::Make(new ACEMonitor);
    connectionPublisher_->setType(serviceType);
    connectionPublisher_->setPort(port);
    connectionPublisher_->connectToPublishedSignal([this](auto v){publisherPublished(v);});

    // Attempt to publish connection information. If there is a name conflict and our name is changed by
    // Zeroconf, the callback publisherPublished() will record the new name and invoke the serviceNameChanged()
    // method.
    //
    if (! connectionPublisher_->publish(serviceName_, false)) {
	LOGERROR << "failed to publish connection with name: " << serviceName << " type: " << serviceType
                 << std::endl;
	return false;
    }

    if (activate(threadFlags, 1, 0, priority) == -1) {
	LOGFATAL << "failed to start remote controller thread - " << std::endl;
	return false;
    }

    return true;
}

void
RemoteControllerBase::publisherPublished(bool ok)
{
    if (ok && connectionPublisher_->getName() != serviceName_) {
	serviceName_ = connectionPublisher_->getName();
	serviceNameChanged(serviceName_);
    }
}

bool
RemoteControllerBase::stop()
{
    Logger::ProcLog log("stop", Log());
    LOGINFO << std::endl;

    if (! running_) {
	LOGERROR << "not running" << std::endl;
	return false;
    }

    // Stop the publication of the XML-RPC service connection information, and delete the publisher
    //
    connectionPublisher_->stop();
    connectionPublisher_.reset();

    // Tell the service thread to stop processing, then wait for the thread to stop.
    //
    running_ = 0;

    // Tell the XML-RPC server to shutdown. However, this won't take affect until something triggers the server
    // to do some work. So, create a bogus connection to the server.
    //
    rpcServer_->shutdown();
    ACE_SOCK_Stream bogus;
    ACE_SOCK_Connector(bogus, address_);

    // Now wait for the service thread to exit. We *must* do this in order to be sure that no XML-RPC requests
    // are in progress when we delete the XML-RPC server below.
    //
    LOGINFO << "waiting for rpc thread to exit: " << running_ << std::endl;
    if (wait() == -1) {
	LOGERROR << "failed wait() on service thread" << std::endl;
	return false;
    }

    // Delete the XML-RPC server.
    //
    rpcServer_.reset();

    return true;
}

int
RemoteControllerBase::svc()
{
    Logger::ProcLog log("svc", Log());
    LOGINFO << std::endl;

    // Have the XML-RPC server run forever. The close() method above issues an XmlRpcServer::shutdown() to cause
    // the server to quit, and a fake connection request to wake up the server to notice that it should exit.
    //
    running_ = 1;
    rpcServer_->work(-1);
    LOGDEBUG << "exited XML-RPC service loop" << std::endl;
    running_ = 0;
    return 0;
}
