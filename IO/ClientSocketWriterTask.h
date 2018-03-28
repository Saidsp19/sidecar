#ifndef SIDECAR_IO_CLIENTSOCKETWRITERTASK_H // -*- C++ -*-
#define SIDECAR_IO_CLIENTSOCKETWRITERTASK_H

#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"
#include "ace/Svc_Handler.h"

#include "IO/IOTask.h"
#include "IO/Writers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** An ACE task that keeps alive a connection to a remote server. Incoming messages to its processing queue are
    written out to the remote server.
*/
class ClientSocketWriterTask : public IOTask {
public:
    using Ref = boost::shared_ptr<ClientSocketWriterTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new ClientSocketWriterTask objects

        \return reference to new ClientSocketWriterTask object
    */
    static Ref Make();

    /** Open a connection to a remote host/port.

        \param key message type key for data sent out port

        \param hostName name of the remote host to connect to

        \param port the port of the remote host to connect to

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& hostName, uint16_t port);

    /** Override of ACE_Task method. The service is begin shutdown. Close the socket connection.

        \param flags if 1, signal service thread to shutdown.

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags = 0);

protected:
    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    ClientSocketWriterTask() : IOTask(), connector_(this) {}

private:
    /** Implementation of Task::deliverDataMessage() method. Places message onto the thread's processing queue.

        \param data message to deliver

        \param timeout amount of time to spend trying to deliver message

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    class Connector;

    /** Helper class that does the writing of data out on a socket connection. There is only one instance of
        this alive, regardless how many times the connection is reestablished.
    */
    class OutputHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> {
    public:
        using Super = ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>;

        /** Log device for objects of this type.

            \return log device
        */
        static Logger::Log& Log();

        /** Constructor.

            \param task the task whose message queue we pull from
        */
        OutputHandler(ClientSocketWriterTask* task = 0) :
            Super(0, task->msg_queue(), task->reactor()), connector_(0), writer_()
        {
        }

        /** Override of ACE_Task method. Setup a new connection to the remote host. This routine is called each
            time a connection is (re)stablished.

            \param arg Connector object that established the connection

            \return 0 if successful, -1 otherwise
        */
        int open(void* arg);

        /** Override of ACE_Task method. Shut the service down.

            \return 0 if successful, -1 otherwise
        */
        int close(u_long flags = 0);

    private:
        /** Override of ACE_Event_Handler method. Notification that the connection to the remote host has
            closed. Set things up to gracefully attempt a reconnection.

            \param handle connection that closed

            \return 0 if successful, -1 otherwise.
        */
        int handle_input(ACE_HANDLE handle);

        /** Override of ACE_Task method. Continuously pulls messages off of the processing queue and sends them
            out on an active socket connection. Runs in a separate thread.

            \return 0 always
        */
        int svc();

        Connector* connector_;   ///< Connector object that manages connections
        TCPSocketWriter writer_; ///< Object that does the actual sending
    };

    /** Helper class that does the connecting to a remote host. If unable to connect, will keep trying every
        second or so. Likewise if the connection drops.
    */
    class Connector : public ACE_Connector<OutputHandler, ACE_SOCK_CONNECTOR> {
    public:
        using Super = ACE_Connector<OutputHandler, ACE_SOCK_CONNECTOR>;

        /** Log device for objects of this type.

            \return log device
        */
        static Logger::Log& Log();

        /** Constructor

            \param task task from which to pull messages to emit.
        */
        Connector(ClientSocketWriterTask* task) :
            Super(task->reactor()), outputHandler_(task), remoteAddress_(), timer_(-1), shutdown_(false)
        {
        }

        /** Atttempt to connect to a remote host using the given address.

            \param remoteAddress address of remote host to connect to

            \param reactor the ACE_Reactor to register with

            \return true if successful, false otherwise
        */
        bool openAndInit(const ACE_INET_Addr& remoteAddress, ACE_Reactor* reactor);

        /** Attempt to reestablish a connection to a remote host.

            \return true if successful, false otherwise
        */
        bool reconnect();

        /** Override of ACE_Task method. Shut down the output handler

            \param flags ignored

            \return 0 if successful, -1 otherwise
        */
        int close();

    private:
        /** Notification that our attempt to connect to a remote server has timed out. Try again.

            \param timeout

            \param arg

            \return
        */
        int handle_timeout(const ACE_Time_Value& timeout, const void* arg);

        /** Attempt to (re)connect to a remote host using the cached connection address.

            \return true if successful, false otherwise
        */
        bool doconnect();

        OutputHandler outputHandler_; ///< Output handler for all connections
        ACE_INET_Addr remoteAddress_; ///< Address for remote host connection
        long timer_;
        bool shutdown_;
    };

    Connector connector_; ///< Object that manages the socket connection
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
