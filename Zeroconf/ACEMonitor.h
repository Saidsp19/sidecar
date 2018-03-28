#ifndef SIDECAR_ZEROCONF_ACEMONITOR_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_ACEMONITOR_H

#include "ace/Event_Handler.h"

#include "Zeroconf/Monitor.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Zeroconf {

/** Concrete implementation of the Monitor class that relies on the ACE infrastructure to determine when a
    Zeroconf object has data available for processing. When there is, it invokes
    Transaction::processConnection() to handle the incoming data.

    The handle_input() method is the notification mechanism that ACE uses when
    there is data available from the file descriptor or socket managed by the
    ACEMonitor.
*/
class ACEMonitor : public ::ACE_Event_Handler, public Monitor {
public:
    /** Obtain the log device for ACEMonitor objects

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor. Sets up the ACE_Event_Handler base class to use the global ACE_Reactor object.
     */
    ACEMonitor();

    /** Destructor.
     */
    ~ACEMonitor();

private:
    /** Implementation of abstract Monitor method. Notification from the monitored object that a service has
        started. Registers ourselves in the ACE framework to receive notifications when data is available on the
        IPC descriptor of the monitored Zerconf object.
    */
    void serviceStarted();

    /** Implementation of abstract Monitor method. Notification from the monitored object that a service is
        stopping. Removes ourselves from the ACE framework so we no longer receive notifications when data is
        available on the IPC descriptor of the monitored Zerconf object.
    */
    void serviceStopping();

    /** Override of ACE_Event_Handler method. Obtain the IPC descriptor from the monitored object

        \return valid IPC descriptor, or -1 if error
    */
    ACE_HANDLE get_handle() const;

    /** Override of ACE_Event_Handler method. Data is available on the IPC descriptor from the monitored object,
        so invoke its processConnection() method to get the data and process it.

        \param fd IPC descriptor with data on it (ignored)

        \return 0 if successful, -1 otherwise
    */
    int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
};

/** Factory of ACEMonitor objects. The Browser object requires a monitor factory so that the browser can give an
    ACEMonitor to each ServiceEntry objects the it creates.
*/
class ACEMonitorFactory : public MonitorFactory {
public:
    using Ref = boost::shared_ptr<MonitorFactory>;

    /** Factory method used to create a reference-counted ACEMonitorFactory object.

        \return new ACEMonitorFactory object
    */
    static Ref Make();

    /** Obtain a new ACEMonitor object.

        \return ACEMonitor object
    */
    Monitor* make() { return new ACEMonitor; }

private:
    /** Constructor. Use Make() factory method to create new ACEMonitorFactory objects.
     */
    ACEMonitorFactory() : MonitorFactory() {}
};

} // namespace Zeroconf
} // namespace SideCar

/** \file
 */

#endif
