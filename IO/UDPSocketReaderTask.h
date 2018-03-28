#ifndef SIDECAR_IO_UDPSOCKETREADERTASK_H // -*- C++ -*-
#define SIDECAR_IO_UDPSOCKETREADERTASK_H

#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"
#include "ace/Svc_Handler.h"

#include "IO/IOTask.h"
#include "IO/Readers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** A socket reader task for UDP messages. For TCP sockets, use either ClientSocketReaderTask or
    ServerSocketReaderTask depending on which end of the connection is responsible for establishing a
    connection.
*/
class UDPSocketReaderTask : public IOTask {
public:
    using Ref = boost::shared_ptr<UDPSocketReaderTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new UDPSocketReaderTask objects

        \return reference to new UDPSocketReaderTask object
    */
    static Ref Make(size_t messageSize = ACE_DEFAULT_CDR_BUFSIZE);

    /** Open a connection to a remote host/port for UDP data.

        \param key message type key of data coming in

        \param port the port to on the local host to read from

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, uint16_t port);

    /** The service is being shutdown. Override of ACE_Task method. Close the socket connection.

        \param flags if 1, signal service thread to shutdown; if 0, then the
        thread is exiting

        \return -1 if failure
    */
    int close(u_long flags = 0);

protected:
    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    UDPSocketReaderTask(size_t messageSize);

private:
    /** Implementation of Task::deliverDataMessage() method.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    /** Obtain the device handle for the UDP socket reader.

        \return device handle
    */
    ACE_HANDLE get_handle() const { return reader_.getDevice().get_handle(); }

    /** Thread processing routine. Override of ACE_Task method. Fetches complete messages from the UDP socket
        reader, and posts them to the next processing module using the ACE_Task::put_next() method.

        \return
    */
    int handle_input(ACE_HANDLE handle);

    UDPSocketReader reader_;
};

using UDPSocketReaderTaskModule = TModule<UDPSocketReaderTask>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
