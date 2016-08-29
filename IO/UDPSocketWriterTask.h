#ifndef SIDECAR_IO_UDPSOCKETWRITERTASK_H // -*- C++ -*-
#define SIDECAR_IO_UDPSOCKETWRITERTASK_H

#include "IO/IOTask.h"
#include "IO/Module.h"
#include "IO/Writers.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** A socket writer task for UDP messages.
 */
class UDPSocketWriterTask : public IOTask
{
public:
    using Ref = boost::shared_ptr<UDPSocketWriterTask>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new UDPSocketWriterTask objects

        \return reference to new UDPSocketWriterTask object
    */
    static Ref Make();

    /** Open a connection to a remote host/port for UDP data.

	\param key message type key of data coming in

        \param port the port to on the local host to read from

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& host,
                     uint16_t port);

protected:

    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
	methods.
    */
    UDPSocketWriterTask();

private:

    /** Override of IO::Task method. We don't know if there is anyone listening, so we always require data to
        emit.

	\return true
    */
    bool calculateUsingDataValue() const { return true; }

    /** Implementation of Task::deliverDataMessage() method.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    /** Obtain the device handle for the UDP socket writer.

        \return device handle
    */
    ACE_HANDLE get_handle() const { return writer_.getDevice().get_handle(); }

    UDPSocketWriter writer_;
};

using UDPSocketWriterTaskModule = TModule<UDPSocketWriterTask>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
