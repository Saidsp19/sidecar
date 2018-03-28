#ifndef SIDECAR_ZEROCONF_MONITOR_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_MONITOR_H

#include "boost/shared_ptr.hpp"

namespace SideCar {
namespace Zeroconf {

class Transaction;

/** Abstract base class that monitors the running status of a Zerconf object such as Publisher or Browser. The
    monitor receives notifications when a Zeroconf service starts or stops. Derived classes must define the
    serviceStarted() and serviceStopping() routines.
*/
class Monitor {
public:
    /** Destructor. Here to silence obnoxious GCC warnings about virtual methods without a virtual destructor.
     */
    virtual ~Monitor() {}

protected:
    /** Constructor. Begin by not monitoring anything.
     */
    Monitor() : monitored_(0) {}

    /** Notification from the monitored object that a service has started. At this point, calls to
        getMonitored() shall return valid values.
    */
    virtual void serviceStarted() = 0;

    /** Notification from the monitored object that a service is about to stop. At this point, calls to
        getMonitored() shall still return valid values, but after this routine exits, they will not.
    */
    virtual void serviceStopping() = 0;

    /** Obtain the monitored object. NOTE: this value must NOT be held by others since Transaction objects are
        reference-counted, and may be destroyed at any time.

        \return monitored object.
    */
    Transaction* getMonitored() const { return monitored_; }

private:
    void setMonitored(Transaction* monitored) { monitored_ = monitored; }

    Transaction* monitored_; ///< Object being monitored

    friend class Transaction; // access to serviceStarted / serviceStopping
};

/** Abstract base class for a Monitor factory. Instances create new Monitor objects to detect when a Transaction
    object needs processing time. Derived classes must define the make() method.
*/
class MonitorFactory {
public:
    using Ref = boost::shared_ptr<MonitorFactory>;

    /** Destructor.
     */
    virtual ~MonitorFactory(){};

    /** Obtain a new Monitor object. Derived classes must define.

        \return Monitor object
    */
    virtual Monitor* make() = 0;

protected:
    /** Constructor.
     */
    MonitorFactory() {}
};

} // end namespace Zeroconf
} // end namespace SideCar

/** \file
 */

#endif
