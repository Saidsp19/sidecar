#ifndef SIDECAR_ZEROCONF_BROWSER_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_BROWSER_H

#include <dns_sd.h>
#include <map>
#include <string>
#include <vector>

#include "boost/shared_ptr.hpp"
#include "boost/signals2.hpp"

#include "Zeroconf/ServiceEntry.h"
#include "Zeroconf/Transaction.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Zeroconf {

/** Zeroconf browser for information regarding available SideCar services. Each Browser instance maintains a
    connection to the multicast DNS (mdsn) server running locally. The mdns server notifies the browser when new
    services become available or die via the BrowseCallback() class method. The browser in turn notifies clients
    that registered themselves with the connectToFoundSignal() and connectToLostSignal() methods.

    Each found service is represented by a ServiceEntry object. The life-time of the ServiceEntry object is the
    time between found and lost notifications; after the lost notification signal, clients must no longer access
    in any way the ServiceEntry object revealed in the lost notification.

    Publishing information to a mdns server is performed by the Publisher class.
*/
class Browser : public Transaction {
    using Super = Transaction;

public:
    using Ref = boost::shared_ptr<Browser>;

    using ServiceEntryVector = std::vector<ServiceEntry::Ref>;
    using ServiceEntryMap = std::map<std::string, ServiceEntry::Ref>;

    using Signal = boost::signals2::signal<void(const ServiceEntryVector&)>;
    using SignalProc = Signal::slot_function_type;
    using SignalConnection = boost::signals2::connection;

    /** Log device for all Publisher objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method used to create a new Browser instance. Browses in the default domain ('.'), and on all
        network interfaces.

        \param monitorFactory creator of Monitor objects for ServiceEntry objects

        \param type what to browse for

        \return reference to new Browser
    */
    static Ref Make(const MonitorFactory::Ref& monitorFactory, const std::string& type);

    /** Destructor. Stops the browser if not already stopped.
     */
    ~Browser();

    /** Obtain the name of the publisher data type to browse for.

        \return publisher data type
    */
    const std::string& getType() const { return type_; }

    /** Obtain the name of the domain to search in. The default value is "local."

        \return domain name
    */
    const std::string& getDomain() const { return domain_; }

    /** Obtain the index of the interface to browse on. The default value is zero, which browses on all active
        interfaces. Use if_indextoname() to convert non-zero index value into a name value.

        \return interface index
    */
    uint32_t getInterface() const { return interface_; }

    /** Change the type of the service being browsed for. Note that the change does not affect an active
        browser; one must stop() and then start() to have the change toke affect.

        \param type the new service type
    */
    void setType(const std::string& type) { type_ = type; }

    /** Change the domain of the service being browsed for. Note that the change does not affect any publishing
        that is in effect; one must stop() and then start() to have the change take effect.

        \param domain the new service domain
    */
    void setDomain(const std::string& domain) { domain_ = domain; }

    /** Change the interface to browse on. Hosts may have more than one network interface active. The default
        behavior of mdns is to publish connection information on all active interfaces. Use this method to
        restrict publishing to a specific network interface.

        \param interface network interface to use
    */
    void setInterface(uint32_t interface) { interface_ = interface; }

    /** Connect an observer to the FoundSignal signal to receive a notification when a new service has been
        found by the browser. The observer must be a function object that takes three parameters:

        - Browser::Ref - the browser that found the service
        - ServiceEntry::Ref - the service that waS found
        - bool - true if there are more notifications on the way

        To disconnect, simply invoke the disconnect() method of the returned object.

        \param observer object to receive notifications

        \return object identifying the signal connection
    */
    SignalConnection connectToFoundSignal(SignalProc observer) { return foundSignal_.connect(observer); }

    /** Connect an observer to the LostSignal signal to receive a notification when a service has been dropped.
        The observer must be a function object that takes three parameters:

        - Browser::Ref - the browser that sent the notification
        - ServiceEntry::Ref - the service that was lost
        - bool - true if there are more notifications on the way

        To disconnect, simply invoke the disconnect() method of the returned object.

        \param observer object to receive notifications

        \return object identifying the signal connection
    */
    SignalConnection connectToLostSignal(SignalProc observer) { return lostSignal_.connect(observer); }

    /** Start the browser. To stop, invoke stop().

        \return true if successful, false otherwise
    */
    bool start();

private:
    /** Constructor. Restricted to derived classes and to the Make() factory method.
     */
    Browser(const MonitorFactory::Ref& monitorFactory, const std::string& type, const std::string& domain,
            uint32_t interface);

    /** Process a DNSServiceBrowse response from the DNSSD server. If no error was reported, it notifies the
        appropriate set of observers based on the added parameter.

        \param err error value from the callback

        \param name the name of the service that was found

        \param type the Zeroconf type of the service that was found

        \param domain the domain in which the service is running

        \param interface the interface to use to reach the service

        \param added true if service was found, false if service is gone

        \param moreToCome true if there are more service notifications to come
    */
    void processResponse(DNSServiceErrorType err, const char* name, const char* type, const char* domain,
                         uint32_t interface, bool added, bool moreToCome);

    MonitorFactory::Ref monitorFactory_;
    std::string type_;   ///< The Zeroconf type to browse for
    std::string domain_; ///< The domain in which to browse
    uint32_t interface_; ///< The interface on which to browse
    Signal foundSignal_; ///< Signal for found service notifications
    Signal lostSignal_;  ///< Signal for removed service notifications

    /** Collection of all found ServiceEntry objects. The key is a string containing the name, type, domain, and
        interface index.
    */
    ServiceEntryMap found_;

    /** Pending ServiceEntry items for 'found' notification. Browser groups all new entries uncovered by
        processResponse() until that method's moreToCome value is false, at which point it emits the found
        signal with this vector.
    */
    ServiceEntryVector finding_;

    /** Pending ServiceEntry items for 'lost' notification. Browser groups all lost entries uncovered by
        processResponse() until that method's moreToCome value is false, at which point it emits the lost signal
        with this vector.
    */
    ServiceEntryVector losing_;

    /** DNSServiceBrowse callback. Invoked when the DNSSD server has service information to report.

        \param ref reference to the browse request

        \param flags request flags (ignored)

        \param interface ID of the interface on which the browsing is active

        \param err result of the call

        \param name name of the service

        \param type type of the service

        \param domain domain in which the service resides

        \param context pointer to the Browser object that called DNSServiceBrowse.
    */
    static void BrowseCallback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface, DNSServiceErrorType err,
                               const char* name, const char* type, const char* domain, void* context);
};

} // end namespace Zeroconf
} // end namespace SideCar

/** \file
 */

#endif
