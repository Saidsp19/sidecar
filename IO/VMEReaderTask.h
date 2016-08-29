#ifndef SIDECAR_IO_VMEREADERTASK_H // -*- C++ -*-
#define SIDECAR_IO_VMEREADERTASK_H

#include "IO/IOTask.h"
#include "IO/Module.h"
#include "IO/Readers.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** A socket reader task for UDP messages. For TCP sockets, use either ClientSocketReaderTask or
    ServerSocketReaderTask depending on which end of the connection is responsible for establishing a
    connection.
*/
class VMEReaderTask : public IOTask
{
public:
    using Ref = boost::shared_ptr<VMEReaderTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new VMEReaderTask objects

        \return reference to new VMEReaderTask object
    */
    static Ref Make();

    /** Open a connection to a remote host/port for UDP data.

	\param interface name of the interface to listen on

        \param port the port of the remote host to connect to

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& host, uint16_t port,
                     int bufferSize = 0);

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
    VMEReaderTask();

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

    int handle_input(ACE_HANDLE device = ACE_INVALID_HANDLE);

    MulticastSocketReader reader_;
};

using VMEReaderTaskModule = TModule<VMEReaderTask>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
