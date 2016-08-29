#ifndef SIDECAR_IO_TCPINPUTHANDLER_H // -*- C++ -*-
#define SIDECAR_IO_TCPINPUTHANDLER_H

#include "ace/Svc_Handler.h"

#include "IO/Readers.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

class IOTask;
class TCPConnector;

class TCPInputHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM,ACE_NULL_SYNCH>
{
    using Super = ACE_Svc_Handler<ACE_SOCK_STREAM,ACE_NULL_SYNCH>;
public:

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    TCPInputHandler(IOTask* task = 0)
	: Super(), connector_(0), task_(task), reader_() {}

    /** Sets up the SocketReader instance to use the socket connection to the client. Override of ACE_Task
	method.

	\param arg ACE_Connector object that did the connecting

	\return -f if failed
    */
    int open(void* arg = 0);

    /** Close the connection. Override of ACE_Task method.

	\param flags ignored

	\return -1 if failed
    */
    int close(u_long flags = 0);

    /** Gives its SocketReader object a chance to fetch data from the socket, and if there is a finished message
	available, it will pass it in a call to ServerSocketReaderTask::put(). NOTE: the return of 1 when
	successful tells the ACE_Reactor object to revisit this handler before doing another ::select call.
	Override of ACE_Event_Handler method.

	\param device socket connection (ignored)

	\return 0 or 1 if successful, -1 otherwise
    */
    int handle_input(ACE_HANDLE device = ACE_INVALID_HANDLE);

    /** Notification that the remote side closed the connection. Override of ACE_Event_Handler method.

	\param device device descriptor of the connection that closed

	\param mask registration mask of the device in the ACE_Reactor

	\return -1 if failed
    */
    int handle_close(ACE_HANDLE device = ACE_INVALID_HANDLE,
                     ACE_Reactor_Mask mask = 0);

private:
    TCPConnector* connector_;
    IOTask* task_;
    TCPSocketReader reader_; ///< Object that does the actual reading
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
