#ifndef SIDECAR_IO_MULTICASTDATAPUBLISHER_H // -*- C++ -*-
#define SIDECAR_IO_MULTICASTDATAPUBLISHER_H

#include <map>
#include <string>

#include "ace/SOCK_Dgram.h"

#include "IO/DataPublisher.h"
#include "IO/Module.h"
#include "IO/Writers.h"
#include "IO/ZeroconfRegistry.h"
#include "Time/TimeStamp.h"
#include "Zeroconf/Browser.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Zeroconf { class Publisher; }
namespace IO {

/** Publisher of data using UDP multicast transport. Relies on MulticastSocketWriter to do the actual delivery
    work. Creates a separate thread that takes messages from the internal message queue and sends them to the
    MulticastSocketWriter object.

    Opens a separate UDP socket to receive heart-beat messages from external subscribers. It processes
    heart-beat data inside the handle_input() method (an ACE_Event_Handler overload). It manages a mapping of
    subscriber addresses to heart-beat receipt timestamps, and periodically scans the mapping for stale entries.

    There are two types of heart-beat messages: 'HI' and 'BYE'. A subscriber sends the first once it establishes
    connection with the publisher, and every 5 seconds or so afterwards while it remains connected. When the
    subscriber closes the multicast connection, it sends a 'BYE' heart-beat so that the publisher will know
    immediately that it has one less subscriber. If for some reason, the 'BYE' does not make it through to the
    publisher, the periodic scan mentioned above will take care of it.
*/
class MulticastDataPublisher : public DataPublisher, public ZeroconfTypes::Publisher
{
    using Super = DataPublisher;
public:
    using Ref = boost::shared_ptr<MulticastDataPublisher>;
    using ServiceEntryVector = Zeroconf::Browser::ServiceEntryVector;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new MulticastDataPublisher objects

        \return reference to new DataPublisher object
    */
    static Ref Make();

    /** Destructor. Shuts down the subscriber browser we use to know when a subscriber is wanting our data.
     */
    ~MulticastDataPublisher();

    /** Open a server connection on a particular port.

	\param key message type key of published data

	\param serviceName Zeroconf name of service publishing the data

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& serviceName, const std::string& ip,
                     long threadFlags = kDefaultThreadFlags, long threadPriority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Override of DataPublisher method. The service is being shutdown.

        \param flags if 1 module is shutting down

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags = 0);

protected:

    /** Constructor. Does nothing -- like most ACE classes, all initialization is done in the init and open
        methods.
    */
    MulticastDataPublisher();

    /** Implementation of Task::deliverDataMessage() interface.

        \param data message to deliver

        \param timeout amount of time to spend trying to deliver the message

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout = 0);

    /** Override of DataPublisher method. Uses the new service name as the basis for our own task name, and
        calls setTaskName() with the new value.

        \param serviceName the new name to use
    */
    void setServiceName(const std::string& serviceName);

    /** Override of DataSubscriber method. Invoked by ACE_Reactor when a scheduled timer times out. Scans the
        existing entries in the HeartBeatMap map, looking for and removing stale entries.

        \param duration ignored

        \param arg value used to distinguish between timer events

        \return 0
    */
    int handle_timeout(const ACE_Time_Value& duration, const void* arg);

private:

    /** Override of ACE_Event_Handler method. Invoked by ACE_Reactor when UDP data is available on the given
        socket handle.

        \param handle socket handle containing the data

        \return 
    */
    int handle_input(ACE_HANDLE handle);

    /** Override of DataPublisher method. Starts a new thread to handle messages added to our input message
        queue.
    */
    void publishSucceeded();

    bool calculateUsingDataValue() const;

    /** Override of ACE_Task method. Routine that runs in a separate thread. Takes messages off of the input
        message queue and hands them to our MulticastSocketWriter object.
    */
    int svc();

    UDPSocketWriter writer_;
    long threadFlags_;
    long threadPriority_;
    ACE_SOCK_Dgram heartBeatReader_;
    long timer_;
    using HeartBeatMap = std::map<std::string,Time::TimeStamp>;
    HeartBeatMap heartBeats_;
};

using MulticastDataPublisherModule = TModule<MulticastDataPublisher>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
