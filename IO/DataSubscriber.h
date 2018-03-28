#ifndef SIDECAR_IO_DATASUBSCRIBER_H // -*- C++ -*-
#define SIDECAR_IO_DATASUBSCRIBER_H

#include "IO/IOTask.h"
#include "Zeroconf/Browser.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** A variant of IOTask which acts as the subscriber part in a publisher/subscriber relationship. A subscriber
    has a service name that it looks for using a Zeroconf::Browser. When a DataPublisher comes on line and
    publishes its Zerconf information, the DataSubscriber's browser informs it of the new service, and the
    DataSubscriber then invokes ClientSocketReaderTask::open() to attempt a connection. Whenever the
    DataPublisher quits, its Zeroconf information becomes invalidated, and the Zeroconf::Browser object informs
    its DataSubscriber that the service is no longer available.
*/
class DataSubscriber : public IOTask {
    using Super = IOTask;

public:
    using Ref = boost::shared_ptr<DataSubscriber>;
    using ServiceEntryVector = Zeroconf::Browser::ServiceEntryVector;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Prepare to subscribe to a data publisher with a given service name. Starts a Zeroconf::Browser to watch
        for DataPublisher objects bearing the service name. Only after a DataPublisher object is found and
        resolved is the ClientSocketReaderTask::open() method invoked.

        \param key message type key of data coming in

        \param serviceName Zeroconf name of service publishing the data

        \return true if successful, false otherwise
    */
    bool openAndInit(const std::string& key, const std::string& serviceName, const std::string& type,
                     int interface = 0);

    /** Shut down the subscriber. Override of Task::close(). Stops the Zeroconf publisher browser.

        \param flags non-zero if task is shutting down

        \return 0 if successful, -1 otherwise
    */
    int close(u_long flags = 0) override;

    /** Obtain information about the publisher we are trying to subscribed to. NOTE: this may return an invalid
        reference if there is no service available.

        \return Zeroconf::ServiceEntry reference
    */
    Zeroconf::ServiceEntry::Ref getServiceEntry() const { return service_; }

    /** Obtain the service name of the current publisher we are subscribed to.

        \return service name
    */
    const std::string& getServiceName() const { return serviceName_; }

    /** Restart the service and the Zeroconf browser that looks for publishers to connect to.
     */
    void restartBrowser();

protected:
    /** Constructor.
     */
    DataSubscriber();

    /** Override of Task::doProcessingStateChange method. Invokes setError() if there is no suitable publisher
        to be found.

        \param msg new state to change to

        \return true if successful, false otherwise
    */
    bool doProcessingStateChange(const ProcessingStateChangeRequest& msg) override;

    /** Event handler called when a Zeroconf ServiceEntry object becomes resolved. Derived classes must
     * implement.

     \param service the ServiceEntry that was resolved
    */
    virtual void resolvedService(const Zeroconf::ServiceEntry::Ref& service) = 0;

    /** Event handler called when the publisher we are currently subscribed to no longer exists. Derived classes
        must implement.
    */
    virtual void lostService() = 0;

    /** Set the name of the publisher we are subscribed to.

        \param serviceName the name of the publisher
    */
    virtual void setServiceName(const std::string& serviceName);

protected:
    /** Override of ACE_Event_Handler method. Invoked by ACE_Reactor when a scheduled timer times out. Starts
        the Zeroconf::Browser.

        \param duration ignored

        \param arg value used to distinguish between timer events

        \return 0
    */
    int handle_timeout(const ACE_Time_Value& duration, const void* arg) override;

private:
    /** Notification from a Zeroconf::Browser that one or more DataPublisher objects have come alive.

        \param services vector of services that have been found
    */
    void foundNotification(const ServiceEntryVector& services);

    /** Notification from a Zeroconf::Browser that one or more DataPublisher objects have disappeared (died).

        \param services vector of services that have been lost
    */
    void lostNotification(const ServiceEntryVector& services);

    /** Notification from a Zeroconf::ServiceEntry that its connection information (host + port) have been
        obtained and stored in a Zeroconf::ResolvedEntry object.

        \param service object containing the connection information
    */
    void resolvedNotification(const Zeroconf::ServiceEntry::Ref& service);

    /** Implementation of Task API. Forwards any incoming data messages to the next task in the stream.

        \param data incoming data message to process

        \param timeout amount of time to wait to deliver before failing

        \return 0 if successful, -1 otherwise
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout = 0) override
    {
        return put_next(data, timeout) != -1;
    }

    Zeroconf::Browser::Ref browser_;
    std::string serviceName_;
    Zeroconf::ServiceEntry::SignalConnection resolvedSignalConnection_;
    Zeroconf::ServiceEntry::Ref service_;
    long timerId_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
