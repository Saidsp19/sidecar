#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Message_Queue.h"
#include "ace/Reactor.h"

#include "IO/Module.h"
#include "Logger/Log.h"

#include "MessageManager.h"
#include "ClientSocketWriterTask.h"

using namespace SideCar::IO;

Logger::Log&
ClientSocketWriterTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ClientSocketWriterTask");
    return log_;
}

ClientSocketWriterTask::Ref
ClientSocketWriterTask::Make()
{
    Ref ref(new ClientSocketWriterTask);
    return ref;
}

Logger::Log&
ClientSocketWriterTask::OutputHandler::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ClientSocketWriterTask.OutputHandler");
    return log_;
}

int
ClientSocketWriterTask::OutputHandler::open(void* arg)
{
    Logger::ProcLog log("open", Log());
    LOGINFO << "arg: " << arg << " handle: " << get_handle() << std::endl;

    // Initialize socket connection.
    //
    connector_ = static_cast<Connector*>(arg);

    // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers. Apparently,
    // !!! mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management. However, we still need
    // !!! this on Mac OS X (Darwin)
    //
#ifdef darwin
    int value = 64 * 1024;	// 64K IO buffer
    if (peer().set_option(SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) == -1) {
        LOGERROR << "failed to set SO_SNDBUF to " << value << std::endl;
    }
#endif

    // Register for notification when the socket in closed.
    //
    if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
        LOGERROR << "failed to register input handler" << std::endl;
        return -1;
    }

    writer_.getDevice().set_handle(get_handle());

    // Only execute the following if the message queue was not in the PULSED state, which happens when we receive
    // notification in handle_input() that the server connection has closed.
    //
    if (msg_queue()->activate() == ACE_Message_Queue_Base::ACTIVATED) {
        LOGINFO << "activating" << std::endl;
        return activate();
    }

    return 0;
}

int
ClientSocketWriterTask::OutputHandler::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;

    if (flags && get_handle() != ACE_INVALID_HANDLE) {

        // Deactivate the message queue. This should stop the processing thread.
        //
        msg_queue()->deactivate();
        if (wait() == -1) {
            LOGERROR << "failed to join to service thread" << std::endl;
            return -1;
        }
    }

    LOGDEBUG << "done" << std::endl;
    return 0;
}

int
ClientSocketWriterTask::OutputHandler::handle_input(ACE_HANDLE handle)
{
    Logger::ProcLog log("handle_input", Log());
    LOGINFO << std::endl;

    // Close the server connection, unregister ourselves for the connection device, and set the message queue to
    // the PULSED state to wake up the threading running our svc() method so that it may attempt to reconnect.
    //
    peer().close();
    reactor()->remove_handler(handle, ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL);
    msg_queue()->pulse();
    return 0;
}

int
ClientSocketWriterTask::OutputHandler::svc()
{
    Logger::ProcLog log("OutputHandler::svc", Log());
    LOGINFO << std::endl;

    ACE_Message_Block* data = 0;

    for (;;) {

        if (data == 0) {

            // If we fail here, it could be that the message queue is in a PULSED state, which indicates that
            // the connection to the server has gone down. This will continue until our open() method is invoked
            // after a new connection has been established.
            //
            if (getq(data) == -1) {
                if (msg_queue()->state() == ACE_Message_Queue_Base::PULSED) {
                    if (! connector_->reconnect()) break;
                }
                else {
                    break;
                }
                continue;
            }
        }
        else {

            MessageManager manager(data);
            if (manager.hasNative()) {

                // If our writer fails to write out data, assume it is a connection problem and try and
                // reconnect. NOTE: the MessageManager always takes ownership of the given data block, so if we
                // fail to write but we resume a connection then we obtain a duplicate of the data block before
                // we loop back. Otherwise, our held data pointer will be destroyed when the MessageManager
                // object goes out of scope.
                //
                if (! writer_.write(manager)) {
                    if (! connector_->reconnect()) {
                        break;
                    }

                    data = data->duplicate();
                    continue;
                }
            }

            data = 0;
        }
    }

    LOGWARNING << "exiting" << std::endl;
    return 0;
}

Logger::Log&
ClientSocketWriterTask::Connector::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ClientSocketWriterTask.Connector");
    return log_;
}

bool
ClientSocketWriterTask::Connector::openAndInit(const ACE_INET_Addr& remoteAddress, ACE_Reactor* reactor)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "address: " << Utils::INETAddrToString(remoteAddress) << std::endl;

    remoteAddress_ = remoteAddress;
    shutdown_ = false;

    if (Super::open() == -1) {
        LOGERROR << "failed to initialize" << std::endl;
        return false;
    }

    if (! doconnect()) {
        LOGERROR << "failed to start connection attempt" << std::endl;
        return false;
    }

    return true;
}

bool
ClientSocketWriterTask::Connector::doconnect()
{
    Logger::ProcLog log("doconnect", Log());
    LOGINFO << "BEGIN" << std::endl;

    if (shutdown_) {
	LOGWARNING << "shutting down" << std::endl;
	return false;
    }

    OutputHandler* tmp = &outputHandler_;
    int rc = Super::connect(tmp, remoteAddress_);
    LOGERROR << "Super::connect: " << rc << " - " << errno << std::endl;

    if (rc == -1) {
	if (timer_ == -1) {
	    LOGERROR << "scheduling connection attempt" << std::endl;
	    const ACE_Time_Value delay(1);
	    const ACE_Time_Value repeat(1);
	    timer_ = reactor()->schedule_timer(this, 0, delay, repeat);
	    if (timer_ == -1) {
		LOGERROR << "failed to schedule timer for reconnect attempt" << std::endl;
		return false;
	    }
	}
    }
    else if (timer_ != -1) {
	LOGERROR << "connected - canceling timer" << std::endl;
	reactor()->cancel_timer(timer_);
	timer_ = -1;
    }

    return true;
}

int
ClientSocketWriterTask::Connector::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    Logger::ProcLog log("handle_timeout", Log());
    LOGERROR << std::endl;
    return doconnect() ? 0 : -1;
}

bool
ClientSocketWriterTask::Connector::reconnect()
{
    Logger::ProcLog log("reconnect", Log());
    LOGINFO << std::endl;
    return doconnect();
}

int
ClientSocketWriterTask::Connector::close()
{
    Logger::ProcLog log("close", Log());
    LOGINFO << std::endl;

    if (timer_ != -1) {
	reactor()->cancel_timer(this);
	timer_ = -1;
    }

    // Close our server socket and our output handler.
    //
    shutdown_ = true;
    return outputHandler_.close(1);
}

bool
ClientSocketWriterTask::openAndInit(const std::string& key, const std::string& hostName, uint16_t port)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << key << ' ' << hostName << ' ' << port << std::endl;

    std::ostringstream os;
    os << "ClientSocketWriterTask(" << key << ',' << hostName << '.' << port << ')';
    setTaskName(os.str());

    setMetaTypeInfoKeyName(key);

    if (! reactor()) reactor(ACE_Reactor::instance());

    ACE_INET_Addr remoteAddress;
    remoteAddress.set(port, hostName.c_str(), 1, AF_INET);
    if (! connector_.openAndInit(remoteAddress, reactor())) {
        LOGERROR << "failed Connector::init" << std::endl;
        return false;
    }

    return true;
}

bool
ClientSocketWriterTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return putq(data, timeout) != -1;
}

int
ClientSocketWriterTask::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;
    Task::close(flags);
    return connector_.close();
}
