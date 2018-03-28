#ifndef SIDECAR_ZEROCONF_ASIOMONITOR_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_ASIOMONITOR_H

#include "boost/asio.hpp"
#include "boost/shared_ptr.hpp"

#include "Zeroconf/Monitor.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Zeroconf {

/** Concrete implementation of the Monitor class that relies on the Boost ASIO infrastructure to determine when
    a Zeroconf socket has data available for processing. When there is, it invokes
    Transaction::processConnection() to handle the incoming data.
*/
class ASIOMonitor : public Monitor {
public:
    /** Obtain the log device for ASIOMonitor objects

        \return log device
    */
    static Logger::Log& Log();

    ~ASIOMonitor();

    /** Constructor.
     */
    ASIOMonitor(boost::asio::io_service& ioService);

private:
    /** Implementation of abstract Monitor method. Notification from the monitored object that a service has
        started. Registers ourselves in the ASIO framework to receive notifications when data is available on
        the IPC descriptor of the monitored Zerconf object.
    */
    void serviceStarted();

    /** Implementation of abstract Monitor method. Notification from the monitored object that a service is
        stopping. Removes ourselves from the ASIO framework so we no longer receive notifications when data is
        available on the IPC descriptor of the monitored Zerconf object.
    */
    void serviceStopping();

    struct Private;
    boost::shared_ptr<Private> p_; ///< !!! Must be shared (not unique) due to ASIO async processing.
};

/** Factory of ASIOMonitor objects.
 */
class ASIOMonitorFactory : public MonitorFactory {
public:
    using Ref = boost::shared_ptr<ASIOMonitorFactory>;

    /** Factory method used to create a reference-counted ASIOMonitorFactory object.

        \return new ASIOMonitorFactory object
    */
    static Ref Make(boost::asio::io_service& ios);

    /** Obtain a new ASIOMonitor object.

        \return ASIOMonitor object
    */
    Monitor* make() { return new ASIOMonitor(ios_); }

private:
    /** Constructor. Use Make() factory method to create new ASIOMonitorFactory objects.
     */
    ASIOMonitorFactory(boost::asio::io_service& ios) : MonitorFactory(), ios_(ios) {}

    boost::asio::io_service& ios_;
};

} // namespace Zeroconf
} // namespace SideCar

/** \file
 */

#endif
