#ifndef SIDECAR_IO_MULTICASTDATASUBSCRIBER_H // -*- C++ -*-
#define SIDECAR_IO_MULTICASTDATASUBSCRIBER_H

#include "ace/SOCK_Dgram.h"
#include "ace/Thread_Mutex.h"

#include "IO/DataSubscriber.h"
#include "IO/Module.h"
#include "IO/ZeroconfRegistry.h"
#include "Zeroconf/Publisher.h"

namespace Logger { class Log; }

namespace SideCar {
namespace IO {

/** Subscriber of data using the UDP multicast transport. Relies on a MulticastSocketReader object to do the
    receiving. This class manages the connection state based on Zeroconf information.
*/
class MulticastDataSubscriber : public DataSubscriber, public ZeroconfTypes::Subscriber
{
    using Super = DataSubscriber;
public:
    using Ref = boost::shared_ptr<MulticastDataSubscriber>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method for creating new MulticastDataSubscriber objects

        \return reference to new MulticastDataSubscriber object
    */
    static Ref Make();

    /** Destructor. Shuts down the publisher if it is still running.
     */
    ~MulticastDataSubscriber();

    /** Prepare to subscribe to a data publisher with a given service name. Starts a Zeroconf::Browser to watch
	for DataPublisher objects bearing the service name. Only after a DataPublisher object is found and
	resolved is the ClientSocketReaderTask::open() method invoked.

	\param key message type key of data coming in

	\param serviceName Zeroconf name of service publishing the data

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& serviceName, int bufferSize = 64 * 1024,
                     int interface = 0, long threadFlags = kDefaultThreadFlags,
                     long threadPriority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Override of DataSubscriber method. The service is being shutdown.

        \param flags if 1 module is shutting down

        \return 0 if successful, -1 otherwise.
    */
    int close(u_long flags = 0);

    /** Override of Task method. Enable the data flow valve when connected to a publisher.

        \param state true if using data from upstream tasks
    */
    void setUsingData(bool state);

protected:

    /** Constructor.
     */
    MulticastDataSubscriber();

    /** Override of DataSubscriber method. Updates the task name with the service name.

        \param serviceName 
    */
    void setServiceName(const std::string& serviceName);

private:

    /** Implementation of DataSubscriber API. Notification that a ServiceEntry has host/port connection
        information. Updates the address_ attribute with the information, and attempts to join to the multicast
        channel.

        \param service the service that was resolved
    */
    void resolvedService(const Zeroconf::ServiceEntry::Ref& service);

    /** Implementation of DataSubscriber API. Notification that a service that was used for a multicast
	connection is no longer available. Shuts down the connection.
    */
    void lostService();
    
    /** Attempt to join the multicast channel indicated by the last resolvedService() call.

	\return true if successful
    */
    void attemptConnection();

    /** Override of ACE_Event_Handler method. Notification that a timer has timed out. Invokes
        attemptConnection() to try and establish a connection to the multicast channel.

        \param duration ignored

        \param arg ignored

        \return 0
    */
    int handle_timeout(const ACE_Time_Value& duration, const void* arg);

    /** Shut down any active reader.
     */
    void stopReader();

    void sendHeartBeat(const char* msg) const;

    void startTimer(int intervalSeconds);

    void stopTimer();

    ACE_Thread_Mutex mutex_;
    ACE_INET_Addr address_;
    class ReaderThread;
    ReaderThread* reader_;
    int bufferSize_;
    long timer_;
    ACE_INET_Addr heartBeatAddress_;
    ACE_SOCK_Dgram heartBeatWriter_;
    long threadFlags_;
    long threadPriority_;
    bool closing_;
};

using MulticastDataSubscriberModule = TModule<MulticastDataSubscriber>;

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
