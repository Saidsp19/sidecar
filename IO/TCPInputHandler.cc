#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Utils/Format.h"

#include "IOTask.h"
#include "TCPConnector.h"
#include "TCPInputHandler.h"

using namespace SideCar::IO;

Logger::Log&
TCPInputHandler::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.TCPInputHandler");
    return log_;
}

int
TCPInputHandler::open(void* arg)
{
    Logger::ProcLog log("open", Log());
    LOGINFO << arg << ' ' << reactor() << ' ' << task_ << std::endl;

    // Remember the Connector that opened us so that when our connection goes down we can try and reconnect.
    //
    connector_ = static_cast<TCPConnector*>(arg);

    int value = connector_->getMaxSocketBufferSize();
    if (value) {
        LOGDEBUG << "before peer().set_option()" << std::endl;
        peer().set_option(SOL_SOCKET, SO_RCVBUF, &value, sizeof(value));
    }

    // Setup the socket reader to use our socket connection
    //
    reader_.getDevice().set_handle(get_handle());

    // Finally, register with the ACE Reactor for reading.
    //
    LOGDEBUG << "before Super::open()" << std::endl;
    if (Super::open(arg) == -1) {
        LOGERROR << "failed to open input handler" << std::endl;
        return -1;
    }

    LOGDEBUG << "EXIT" << std::endl;
    return 0;
}

int
TCPInputHandler::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << flags << ' ' << connector_ << std::endl;

    int rc = 0;

    // Shutdown request from our connector. Execute the following only if we had been previously connected. The
    // ACE_Connector class calls our close() method after each failed connection attempt.
    //
    if (connector_) {
        LOGDEBUG << "removing handler" << std::endl;

        // Forget our connector, so we won't attempt to reconnect when our handle_close() method gets called due
        // to the remove_handler() call below.
        //
        connector_ = 0;
        rc = reactor()->remove_handler(this, ACE_Event_Handler::READ_MASK);
        if (rc == -1) { LOGERROR << "failed remove_handler() - " << Utils::showErrno() << std::endl; }
    }

    return rc;
}

int
TCPInputHandler::handle_input(ACE_HANDLE handle)
{
    Logger::ProcLog log("handle_input", Log());
    LOGINFO << handle << std::endl;

    // If we fail, return -1 to call our handle_close() method, which will attempt to reestablish a connection.
    //
    if (!reader_.fetchInput()) {
        LOGERROR << "failed to read data from socket" << std::endl;
        task_->setError("Failed to read from publisher");
        return -1;
    }

    if (!reader_.isMessageAvailable()) return 0;

    // Got a valid message. Give to the task.
    //
    task_->acquireExternalMessage(reader_.getMessage());

#ifdef FIONREAD

    // See if there is more data available to read from the socket. If so, we return 1 so that ACE_Reactor will
    // call us again before doing another ::select().
    //
    int available = 0;
    if (ACE_OS::ioctl(handle, FIONREAD, &available) == 0 && available) {
        LOGDEBUG << "more to come" << std::endl;
        return 1;
    }

#endif

    return 0;
}

int
TCPInputHandler::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask)
{
    Logger::ProcLog log("handle_close", Log());
    LOGINFO << handle << ' ' << mask << std::endl;

    // Either we failed during a handle_input() or we're being called from ACE_Reactor::remove_handler(). Close
    // our socket connection. If our close() method has not been called (where set connector_ to NULL), then try
    // and reconnect.
    //
    ACE_OS::closesocket(handle);
    if (connector_) connector_->scheduleConnectionAttempt();
    return 0;
}
