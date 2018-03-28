#ifndef SIDECAR_IO_CLIENTSOCKETREADERTASK_H // -*- C++ -*-
#define SIDECAR_IO_CLIENTSOCKETREADERTASK_H

#include "IO/IOTask.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

class TCPConnector;

/** An ACE task that keeps alive a TCP connection to a remote server. Incoming data from the remote server are
    placed in the processing queue for the next task in the processing stream.
*/
class ClientSocketReaderTask : public IOTask {
public:
    using Ref = boost::shared_ptr<ClientSocketReaderTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new ClientSocketReaderTask objects

        \return reference to new ClientSocketReaderTask object
    */
    static Ref Make();

    /** Open a connection to a remote host/port.

        \param key message type key of data being sent over the port

        \param hostName name of the remote host to connect to

        \param port the port of the remote host to connect to

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& hostName, uint16_t port);

    /** The service is being shutdown. Override of ACE_Task method. Close any existing socket connection, and
        remove our Connector object.

        \param flags if 1, task is shutting down (ignored)

        \return -1 if failed
    */
    int close(u_long flags = 0);

protected:
    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
     * methods.
     */
    ClientSocketReaderTask() : IOTask(), connector_(0) {}

private:
    /** Implementation of Task::deliverDataMessage() method.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout) { return put_next(data, timeout) != -1; }

    TCPConnector* connector_; ///< Object that manages the socket connection
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
