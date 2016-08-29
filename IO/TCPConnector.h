#ifndef SIDECAR_IO_TCPCONNECTOR_H // -*- C++ -*-
#define SIDECAR_IO_TCPCONNECTOR_H

#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"

#include "IO/IOTask.h"
#include "IO/TCPInputHandler.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

class TCPConnector : public  ACE_Connector<TCPInputHandler,ACE_SOCK_CONNECTOR>
{
    using Super = ACE_Connector<TCPInputHandler,ACE_SOCK_CONNECTOR>;
public:

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

	\param task task from which to pull messages to emit.
    */
    TCPConnector(IOTask* task, int maxSocketBufferSize = 0)
	: Super(task->reactor()), task_(task), remoteAddress_(),
	  inputHandler_(task), maxSocketBufferSize_(maxSocketBufferSize),
	  timer_(-1) {}

    /** Atttempt to connect to a remote host using the given address.

	\param remoteAddress address of remote host to connect to

	\param reactor the ACE_Reactor to register with

	\return true if successful, false otherwise
    */
    bool openAndInit(const ACE_INET_Addr& remoteAddress,
                     ACE_Reactor* reactor);

    int getMaxSocketBufferSize() const { return maxSocketBufferSize_; }

    /** Shut down the output handler. Override of ACE_Connector method.

	\return 0 if successful, -1 otherwise
    */
    int close();

    /** Notification that our attempt to connect to a remote server has timed out. Try again.

	\param timeout

	\param arg

	\return
    */
    int handle_timeout(const ACE_Time_Value& timeout, const void* arg);

    /** Attempt to (re)connect to a remote host using the cached connection address. Creates a new InputHandler
	object if the connection succeeds.

	\return true if successful, false otherwise
    */
    bool attemptConnection();

    bool scheduleConnectionAttempt();

private:
    IOTask* task_;
    ACE_INET_Addr remoteAddress_; ///< Address for remote host connection
    TCPInputHandler inputHandler_;
    int maxSocketBufferSize_;
    long timer_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
