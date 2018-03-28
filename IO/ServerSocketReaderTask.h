#ifndef SIDECAR_IO_SERVERSOCKETREADERTASK_H // -*- C++ -*-
#define SIDECAR_IO_SERVERSOCKETREADERTASK_H

#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Svc_Handler.h"

#include "IO/IOTask.h"
#include "IO/Readers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** An ACE service / task that sets up a server-side socket for client connections, managers client connect
    requests, and receives data from clients using a SocketReader object. Incoming messages are given to the
    services' put() method, where they are forwarded to the next task in the processing sream.
*/
class ServerSocketReaderTask : public IOTask {
    using Super = IOTask;

public:
    using Ref = boost::shared_ptr<ServerSocketReaderTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new ServerSocketReaderTask objects

        \return reference to new ServerSocketReaderTask object
    */
    static Ref Make();

    /** Open a server connection on a particular port.

        \param key message type key for data coming over the port

        \param port the port to use for the service

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, uint16_t port = 0);

    /** Override of ACE_Task method. The service is begin shutdown. Close the server socket.

        \param flags if 1 module is shutting down

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags);

    uint16_t getServerPort() const { return port_; }

protected:
    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    ServerSocketReaderTask();

private:
    /** Implementation of Task::deliverDataMessage() method.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    /** Helper class that acquires data from connected clients. A separate thread runs the InputHandler::svc()
        routine, which takes complete messages from a SocketReader. The messages are then passed to the
        SocketReaderTask::put() routine.
    */
    class InputHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> {
    public:
        using Super = ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>;

        /** Override of ACE_Svc_Handler method. Sets up the SocketReader instance to use the socket connection
            to the client.

            \param arg ACE_Acceptor object that did the connecting

            \return 0 if successful, -1 otherwise
        */
        int open(void* arg = 0);
        int close(u_long flags = 0);

    private:
        /** Override of ACE_Event_Handler method. Gives its SocketReader object a chance to fetch data from the
            socket, and if there is a finished message available, it will pass it in a call to
            ServerSocketReaderTask::put(). NOTE: the return of 1 when successful tells the ACE_Reactor object to
            revisit this handler before doing another ::select call.

            \param device socket connection (ignored)

            \return 0 or 1 if successful, -1 otherwise
        */
        int handle_input(ACE_HANDLE device = ACE_INVALID_HANDLE);

        ServerSocketReaderTask* task_; ///< Task to receive message objects
        TCPSocketReader* reader_;      ///< Reader of message objects
    };

    /** Helper class that listens for and processes remote client connection attempts. Each successful client
        connection results in a new InputHandler object to do the data transfer.
    */
    class Acceptor : public ACE_Acceptor<InputHandler, ACE_SOCK_ACCEPTOR> {
    public:
        using Super = ACE_Acceptor<InputHandler, ACE_SOCK_ACCEPTOR>;

        /** Constructor.

            \param task task that receives all incoming messages
        */
        Acceptor(ServerSocketReaderTask* task) : Super(), task_(task) {}

        ServerSocketReaderTask* getTask() const { return task_; }

    private:
        ServerSocketReaderTask* task_; ///< Task to receive message objects
    };

    Acceptor* acceptor_; ///< Socket acceptor for client connections
    uint16_t port_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
