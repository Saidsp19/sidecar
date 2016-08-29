#ifndef SIDECAR_ALGORITHMS_BUGCOLLECTORUTILS_BUGPLOTSUBSCRIBER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_BUGCOLLECTORUTILS_BUGPLOTSUBSCRIBER_H

#include <string>
#include <vector>

#include "ace/Event_Handler.h"
#include "ace/SOCK_Dgram.h"

#include "IO/Readers.h"
#include "Messages/MetaTypeInfo.h"
#include "Zeroconf/Browser.h"

namespace SideCar {
namespace Algorithms {

class BugCollector;

namespace BugCollectorUtils {

/** Multicast data reader of BugPlot messages. Created by a BugPlotSubscriber instance whenever it detects a new
    BugPlot publisher. Forwards each incoming BugPlot message to the BugCollector algorithm it knows about via
    its BugCollector::process() method.
*/
class BugPlotReader : public ACE_Event_Handler
{
public:

    /** Constructor. Only performs simple initialization.

        \param bugCollector the Bugcollector algorithm to forward BugPlot
        messages to
    */
    BugPlotReader(BugCollector& bugCollector);

    /** Attempt to establish a multicast connection to the BugPlot publisher described by the given ServiceEntry
        object.

        \param service description of the BugPlot publisher to subscribe to

        \return true if successful
    */
    bool openAndInit(const Zeroconf::ServiceEntry::Ref& service);

    /** Unsubscribe from the BugPlot publisher.

        \param flags ignored

        \return 
    */
    int close(u_long flags);

    /** Event handler invoked when data is available on the subscription socket.

        \param handle socket identifier

        \return 0 if no error, 1 if more data is available, -1 if an error
    */
    int handle_input(ACE_HANDLE handle);

    /** Obtain the socket descriptor for the connection.

        \return 
    */
    ACE_HANDLE get_handle() const;

    /** Obtain the name of the publisher we are subscribed to

        \return publisher name
    */
    const std::string& getServiceName() const { return serviceName_; }

    /** Send a heartbeat message to the remote bug emitter to keep it sending data to us.

        \param msg the message to send (either "HI" or "BYE")
    */
    void sendHeartBeat(const char* msg = "HI") const;

private:

    BugCollector& bugCollector_;
    std::string serviceName_;
    IO::MulticastSocketReader reader_;
    const Messages::MetaTypeInfo& metaTypeInfo_;

    ACE_INET_Addr heartBeatAddress_;
    ACE_SOCK_Dgram heartBeatWriter_;
};

/** Automatic BugPlot channel subscriber. Creates a Zeroconf::Browser for BugPlot publishers, and creates a new
    BugPlotReader for each one it finds. Note that it will only subscribe to a publisher whose name starts with
    a specific prefix value.
*/
class BugPlotSubscriber : public ACE_Event_Handler
{
public:
    using ServiceEntryVector = Zeroconf::Browser::ServiceEntryVector;

    /** Constructor. Creates a new Zeroconf::Browser and starts it.

        \param bugCollector the algorithm to give to new BugPlotReader objects

        \param prefix the filter to apply for BugPlot publishers
    */
    BugPlotSubscriber(BugCollector& bugCollector, const std::string& prefix);

    /** Destructor. Shut down all BugPlotReader instances.
     */
    ~BugPlotSubscriber();

    /** Obtain the number of active BugPlotReader instances.

        \return 
    */
    size_t size() const { return readers_.size(); }

    /** Change the publisher name filter. NOTE: this will destroy any existing BugPlotReader instances.

        \param value new prefix value
    */
    void setPrefix(const std::string& value);

    /** Override of ACE_Event_Handler::handle_timeout(). Emits heartbeats for all active BugPlotReader objects.

        \param duration ignored

        \param arg ignored

        \return 0
    */
    int handle_timeout(const ACE_Time_Value& duration, const void* arg);

private:

    /** Notification from our Zeroconf::Browser object when a new BugPlot publisher is available.

        \param services list of new publishers
    */
    void foundNotification(const ServiceEntryVector& services);

    /** Notification from our Zeroconf::Browser object when a BugPlot publisher is no longer available.

        \param services list of invalid publishers
    */
    void lostNotification(const ServiceEntryVector& services);

    /** Notificatioin from a Zeroconf::ServiceEntry instance when it has resolved its connection information.

        \param service publisher information
    */
    void resolvedNotification(const Zeroconf::ServiceEntry::Ref& service);

    BugCollector& bugCollector_;
    Zeroconf::Browser::Ref browser_;
    std::vector<BugPlotReader*> readers_;
    std::string prefix_;
    long timer_;
};

} // end namespace BugCollectorUtils
} // end namespace Algorithms
} // end namespace SideCar

#endif
