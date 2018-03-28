#ifndef SIDECAR_ZEROCONF_TRANSACTION_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_TRANSACTION_H

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include <dns_sd.h>

#include "Zeroconf/Monitor.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Zeroconf {

/** Base class for all Zeroconf classes that interact with the DNSSD library. Holds a opaque connection to the
    DNSSD library whenever a transaction with the library is in progress. The class requires external monitoring
    of the DNSSD connection by instances of the Monitor class. The Transaction class contains connection-neutral
    policy.

    Derived classes may override serviceStarted() and serviceStopping() methods to receive notification when
    those events occur. However, the overrides must invoke the methods in this class to maintain proper monitor
    behavior.
*/
class Transaction {
public:
    using Ref = boost::shared_ptr<Transaction>;

    /** Obtain the log device for all Transaction objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Destructor. NOTE: if service is running when this is reached, bad things are probably in the works. At
        this point we cannot dispatch virtual methods correctly, so the best we can do is tell our held Monitor
        object to stop.
    */
    virtual ~Transaction();

    /** Stop a Zeroconf service.

        \return true if successful, false otherwise
    */
    bool stop();

    /** Determine if a Zeroconf service is running.

        \return true if so
    */
    bool isRunning() const { return ref_ != 0; }

    /** Obtain the underlying IPC descriptor used to communicate with the Zeroconf daemon.

        \return valid IPC descriptor, or -1 if not running
    */
    int getConnection() const { return isRunning() ? DNSServiceRefSockFD(ref_) : -1; }

    /** Data is available from the Zeroconf daemon. Let the DNSSD library do its work. NOTE: this should not be
        called unless there really is data available, otherwise it will block until there is data.

        \return true if successful, false if not running or error
    */
    bool processConnection();

protected:
    /** Constructor. Restricted to derived classes.
     */
    Transaction(Monitor* monitor);

    /** Hook for derived classes for notification that a service has started. At this point, a call to
        getConnection() should not return -1.
    */
    virtual void serviceStarted();

    /** Hook for derived classes for notification that a sservice is being stopped. At this point, a call to
        getConnection() still should not return -1, but afterwards it will.
    */
    virtual void serviceStopping();

    /** Set a weak reference to ourselves so that we can give out shared_ptr refences to ourselves.

        \param self reference to initialize with
    */
    void setSelf(const Ref& self) { self_ = self; }

    void finished() { finished_ = true; }

    void finish() { finished_ = true; }

    /** Obtain a shared_ptr reference to ourselves. The template parameter indicates a derived class that is the
        actual type of 'this'.

        \return new shared_ptr reference
    */
    template <typename T>
    boost::shared_ptr<T> getSelf() const
    {
        return boost::dynamic_pointer_cast<T>(self_.lock());
    }

    DNSServiceRef& getDNSServiceRef() { return ref_; }

    const DNSServiceRef& getDNSServiceRef() const { return ref_; }

private:
    std::unique_ptr<Monitor> monitor_;  ///< Active monitor object
    boost::weak_ptr<Transaction> self_; ///< Weak reference to self
    DNSServiceRef ref_;                 ///< Opaque connection handle from DNSSD server
    bool finished_;
};

} // end of namespace Zeroconf
} // end of namespace SideCar

/** \file
 */

#endif
