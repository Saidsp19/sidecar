#ifndef SIDECAR_IO_MULTICASTVMEREADERTASK_H // -*- C++ -*-
#define SIDECAR_IO_MULTICASTVMEREADERTASK_H

#include "IO/IOTask.h"
#include "IO/Module.h"
#include "IO/Readers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** A socket reader task for multicast UDP messages.
 */
class MulticastVMEReaderTask : public IOTask {
    using Super = IOTask;

public:
    using Ref = boost::shared_ptr<MulticastVMEReaderTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new VMEReaderTask objects

        \return reference to new VMEReaderTask object
    */
    static Ref Make();

    void setBufferSize(int bufferSize) { bufferSize_ = bufferSize; }

    /** Open a connection to a remote host/port for UDP data.

        \param host name of the host to subscribe to

        \param port the port of the remote host to connect to

        \param bufferSize value to use for SO_RCVBUV request. Default is 256K.

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& host, uint16_t port, int bufferSize = 0, long threadFlags = kDefaultThreadFlags,
                     long threadPriority = ACE_DEFAULT_THREAD_PRIORITY);

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
    MulticastVMEReaderTask();

private:
    /**

        \return
    */
    int svc();

    void attemptConnection();

    bool scheduleAttempt();

    int handle_timeout(const ACE_Time_Value& duration, const void* arg);

    /** Implementation of Task::deliverDataMessage() method.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    MulticastSocketReader reader_;
    ACE_INET_Addr address_;
    int bufferSize_;
    long timer_;
    long threadFlags_;
    long threadPriority_;
};

using MulticastVMEReaderTaskModule = TModule<MulticastVMEReaderTask>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
