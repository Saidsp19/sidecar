#ifndef SIDECAR_IO_SERVERSOCKETWRITERTASK_H // -*- C++ -*-
#define SIDECAR_IO_SERVERSOCKETWRITERTASK_H

#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Svc_Handler.h"
#include "boost/signals2.hpp"

#include "IO/IOTask.h"
#include "IO/Writers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** An ACE service / task that sets up a server-side socket for client connections, managers client connect
    requests, and send data to clients using a SocketWriter object. Outgoing messages are given to the services'
    put() method, where they are queued for transmission.
*/
class ServerSocketWriterTask : public IOTask {
    using Super = IOTask;

public:
    using Ref = boost::shared_ptr<ServerSocketWriterTask>;
    using ConnectionCountChanged = boost::signals2::signal<void(int)>;
    using ConnectionCountChangedProc = ConnectionCountChanged::slot_function_type;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new ServerSocketWriterTask objects

        \return reference to new ServerSocketWriterTask object
    */
    static Ref Make();

    ~ServerSocketWriterTask();

    /** Open a server connection on a particular port.

        \param key message type key for data going out over the port

        \param port the port to use for the service

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, uint16_t port = 0, int bufferSize = 0,
                     long threadFlags = kDefaultThreadFlags, long threadPriority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Override of ACE_Task method. The service is begin shutdown. Close the server socket.

        \param flags if 1 module is shutting down

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags);

    /** Obtain the port that the server is publishing data on. This is only valid after the open() method
        executes. Useful when the open() method was called with 0 for the port value, allowing the system to
        assign an unused port number to the servier's socket.

        \return server port
    */
    uint16_t getServerPort() const { return port_; }

    /** Obtain the number of active connections to the server.

        \return connection count
    */
    size_t getConnectionCount() const { return clients_.size(); }

    int getBufferSize() const { return bufferSize_; }

    /** Hand a data message to the task to process. Note that since it circumvents the normal message routing
        framework found in Task, this should be used with care.

        \param data message to deliver

        \return true if successful
    */
    bool injectDataMessage(ACE_Message_Block* data) { return deliverDataMessage(data, 0); }

    boost::signals2::connection connectConnectionCountChangedTo(ConnectionCountChangedProc observer)
    {
        return connectionCountChangedSignal_.connect(observer);
    }

protected:
    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    ServerSocketWriterTask();

private:
    /** Implementation of Task::deliverDataMessage() method.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    /** Helper class that acquires data from connected clients. A separate thread runs the InputHandler::svc()
        routine, which takes complete messages from a SocketWriter. The messages are then passed to the
        SocketWriterTask::put() routine.
    */
    class OutputHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> {
        using Super = ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>;

    public:
        /** Log device for objects of this type.

            \return log device
        */
        static Logger::Log& Log();

        OutputHandler() : Super(), task_(0), writer_() {}

        ~OutputHandler();

        /** Override of ACE_Svc_Handler method. Sets up the SocketWriter instance to use the socket connection
            to the client.

            \param arg ACE_Acceptor object that did the connecting

            \return 0 if successful, -1 otherwise
        */
        int open(void* arg = 0);
        int close(u_long flags);
        int put(ACE_Message_Block* data, ACE_Time_Value* timeout = 0) { return putq(data, timeout); }
        int handle_input(ACE_HANDLE handle = ACE_INVALID_HANDLE);

    private:
        int svc();

        ServerSocketWriterTask* task_; ///< Task to receive message objects
        TCPSocketWriter writer_;       ///< Reader of message objects
    };

    friend class OutputHandler;

    void addOutputHandler(OutputHandler* handler);

    void removeOutputHandler(OutputHandler* handler);

    void distribute(ACE_Message_Block* data);

    void updateClients(ACE_Message_Block* data);

    int svc();

    /** Helper class that listens for and processes remote client connection attempts. Each successful client
        connection results in a new OutputHandler object to do the data transfer.
    */
    class Acceptor : public ACE_Acceptor<OutputHandler, ACE_SOCK_ACCEPTOR> {
        using Super = ACE_Acceptor<OutputHandler, ACE_SOCK_ACCEPTOR>;

    public:
        /** Log device for objects of this type.

            \return log device
        */
        static Logger::Log& Log();

        /** Constructor.

            \param task task that receives all incoming messages

        */
        Acceptor(ServerSocketWriterTask* task) : Super(), task_(task) {}

        ~Acceptor();

        ServerSocketWriterTask* getTask() const { return task_; }

        int handle_input(ACE_HANDLE listener);

        int make_svc_handler(OutputHandler*& sh);

    private:
        ServerSocketWriterTask* task_; ///< Task to receive message objects
    };

    Acceptor* acceptor_; ///< Socket acceptor for client connections
    uint16_t port_;
    using OutputHandlerVector = std::vector<OutputHandler*>;
    OutputHandlerVector clients_;
    int bufferSize_;
    long threadFlags_;
    long threadPriority_;
    ConnectionCountChanged connectionCountChangedSignal_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
