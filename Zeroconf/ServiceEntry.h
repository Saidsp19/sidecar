#ifndef SIDECAR_ZEROCONF_SERVICEENTRY_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_SERVICEENTRY_H

#include <dns_sd.h>
#include <string>

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/signals2.hpp"

#include "Zeroconf/ResolvedEntry.h"
#include "Zeroconf/Transaction.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Zeroconf {

/** Information about of a Zeroconf service published by a Publisher object. Contains the name, type, and domain
    of the published service. Also supports resolution of a service into a host and port address, encapsulated
    in a ResolvedEntry object.

    ServiceEntry objects get created by a Browser whenever the multicast DNS server tells it that a new service
    is available, due it using a Publisher to publish its connection information. The ServiceEntry just contains
    the name, type, domain, and the network interface over which the service may be found. However, users must
    invoke the ServiceEntry::resolve() method in order to obtain actual host/port connection information.
*/
class ServiceEntry : public Transaction
{
public:
    using Ref = boost::shared_ptr<ServiceEntry>;
    using SignalConnection = boost::signals2::connection;
    using ResolvedSignal = boost::signals2::signal<void (const ServiceEntry::Ref&)>;
    using ResolvedSignalProc = ResolvedSignal::slot_function_type;

    /** Log device for all ServiceEntry objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method used to create a new ServiceEntry instance

        \param name the name of the service

        \param type the type of the service (eg '_sidecar._tcp')

        \param domain the domainn in which the service resides (eg. 'local.')

        \param interface the system interface on which the service is available

        \return reference to new ServiceEntry
    */
    static Ref Make(Monitor* monitor, const std::string& name, const std::string& type, const std::string& domain,
                    uint32_t interface);

    /** Destructor.
     */
    ~ServiceEntry();

    /** Connect an observer to the ResolvedSignal signal to receive a notification when the ServiceEntry has
        been resolved to a host/port address. The observer must be a function object that takes two parameters:
        - ServiceEntry::Ref - reference to the object doing the signalling - ResolvedEntry::Ref - reference to
        the resolved information. If the resolved request failed, then the reference will be invalid. To
        disconnect, simply invoke the disconnect() method of the returned object.

        \param observer object to receive notifications

        \return object identifying the signal connection
    */
    SignalConnection connectToResolvedSignal(ResolvedSignalProc observer)
        { return resolvedSignal_.connect(observer); }

    /** Obtain the name of the service.

        \return service name
    */
    const std::string& getName() const { return name_; }

    /** Obtain the type of the service.

        \return service type
    */
    const std::string& getType() const { return type_; }

    /** Obtain the domain in which the service resides.

        \return service domain
    */
    const std::string& getDomain() const { return domain_; }

    /** Obtain the interface ID on which one can contact the service.

        \return interface ID
    */
    uint32_t getInterface() const { return interface_; }

    /** Obtain the system's name for the Ethernet interface.

        \return interface name
    */
    std::string getInterfaceName() const;

    /** Begin the resolution process for this ServiceEntry object. Asks the DNSSD server to do the work. When
        the resolution is finished, the ResolvedSignal is emitted.

	\param blocking if true, do not return until the service has resolved

        \return true if successful, false otherwise
    */
    bool resolve(bool blocking = false);

    /** Determine if the service has been resolved. NOTE: it is possible that previously resolved service
        information is no longer accurate. The only way to know is to attempt to connect to it.

        \return 
    */
    bool isResolved() const { return resolved_.get() != 0; }

    /** Obtain a previously-resolved service entry. NOTE: this is only valid if isResolved() returns true, and
        it is unwise to keep a reference of this around.

        \return reference to ResolvedEntry object 
    */
    const ResolvedEntry& getResolvedEntry() const { return *resolved_; }

private:

    /** Process a DNSServiceResolve response from the DNSSD server. This implementation makes or updates the
        held ResolvedEntry object, and then invokes resolvedService(). invokes publishedService().

        \param err result of the call

        \param fullName the fully-qualified Zeroconf name for the service

        \param host the name of the host where the service runs

        \param port the port on the host where the service runs

        \param textSize the number of characters in the mDNS TXT record

        \param textData pointer to the data of the TXT record
    */
    void processResponse(DNSServiceErrorType err, const char* fullName, const char* host, uint16_t port,
                         uint16_t textSize, const unsigned char* textData);

    /** Constructor. Restricted to the Make() factory method.

        \param name name of the service

        \param type type of the service

        \param domain domain in which the service is registered

        \param interface interface on which the service may be connected to
    */
    ServiceEntry(Monitor* monitor, const std::string& name, const std::string& type, const std::string& domain,
                 uint32_t interface);

    std::string name_;		///< Name of the service
    std::string type_;		///< Type of the service
    std::string domain_;	///< Domain of the service
    uint32_t interface_;	///< Interface of the service
    boost::scoped_ptr<ResolvedEntry> resolved_;	///< Resolved info for service
    ResolvedSignal resolvedSignal_; ///< Signal for resolved notifications

    /** DNSServiceResolve callback. Invoked when the DNSSD server has completed a DNSServiceResolve request to
        obtain the host/port information for a service. Updates the resolved_ attribute with information from
        the server. If there was an error, then the resolved_ reference will become invalid. Otherwise, it will
        hold the provided connection information.

        \param ref reference to the resolve request

        \param flags request flags (ignored)

        \param interface ID of the interface the service is on

        \param err result of the call

        \param fullName full name of the service (ignored)

        \param host host name where the service is running

        \param port port on the host where the service is running

        \param textSize number of characters in the TXT record

        \param textData start of the data in the TXT record

        \param context pointer to the ServiceEntry object that called
        DNSServiceResolve
    */
    static void ResolveCallback(DNSServiceRef ref, DNSServiceFlags flags, uint32_t interface,
                                DNSServiceErrorType err, const char* fullName, const char* host, uint16_t port,
                                uint16_t textSize, const unsigned char* textData, void* context);
};

}
}

/** \file
 */

#endif
