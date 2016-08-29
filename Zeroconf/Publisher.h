#ifndef SIDECAR_ZEROCONF_PUBLISHER_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_PUBLISHER_H

#include <dns_sd.h>
#include <map>
#include <string>

#include "boost/shared_ptr.hpp"
#include "boost/signals2.hpp"

#include "Zeroconf/Transaction.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Zeroconf {

/** Zeroconf publisher of information regarding an available service. Each SideCar data publisher uses a
    Zeroconf Publisher to publish its connection information, most importantly the host, port pair that allow
    subscribers to connect to the publisher. Maintains a connection to the multicast DNS (mdns) server running
    locally, and the data publication remains valid for the life of the connection. Supports publishing of
    key/value pairs.

    Accessing published information in a mdns server is performed by the Browser class. Valid published types are
    defined in IO/ZeroconfRegistry.h.
*/
class Publisher : public Transaction
{
    using Super = Transaction;
public:
    using Ref = boost::shared_ptr<Publisher>;
    using SignalConnection = boost::signals2::connection;
    using PublishedSignal = boost::signals2::signal<void (bool)>;
    using PublishedSignalProc = PublishedSignal::slot_function_type;
    using StringMap = std::map<std::string,std::string>;

    /** Log device for all Publisher objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method used to create a new Publisher instance

        \return reference to new Publisher
    */
    static Ref Make(Monitor* monitor);

    /** Obtain the name of the service

        \return service name
    */
    const std::string& getName() const { return name_; }
    
    /** Obtain the service type

        \return service type
    */
    const std::string& getType() const { return type_; }
    
    /** Obtain the domain we are registered in

        \return domain name
    */
    const std::string& getDomain() const { return domain_; }

    /** Obtain the network interface to publish for

        \return network interface index
    */
    uint32_t getInterface() const { return interface_; }

    /** Obtain the port number of our address

        \return port number
    */
    uint16_t getPort() const { return port_; }

    /** Change the type of the service being published. Note that the change does not affect any publishing that
        is in effect; one must stop() and then start() to have the change take effect.

        \param type the new service type
    */
    void setType(const std::string& type) { type_ = type; }

    /** Change the domain of the service being published. Note that the change does not affect any publishing
        that is in effect; one must stop() and then start() to have the change take effect.

        \param domain the new service domain
    */
    void setDomain(const std::string& domain) { domain_ = domain; }

    /** Change the host of the service being published. Does not perform any validation on the value given. Note
        that the change does not affect any publishing that is in effect; one must stop() and then start() to
        have the change take effect.
        
	\param host the new host identifier
    */
    void setHost(const std::string& host) { setTextData("host", host); }

    /** Change the port of the service being published. Note that the change does not affect any publishing that
        is in effect; one must stop() and then start() to have the change take effect.
        
        \param port the number of the socket port on the local host used by the
        service
    */
    void setPort(uint16_t port) { port_ = port; }

    /** Change the interface to publish on. Hosts may have more than one network interface active. The default
        behavior of mdns is to publish connection information on all active interfaces. Use this method to
        restrict publishing to a specific network interface.
        
        \param interface network interface to use
    */
    void setInterface(uint32_t interface) { interface_ = interface; }

    /** Connect an observer to the PublishedSignal signal to receive a notification when the published state has
        changed. The observer must be a function object that takes two parameters:
        
        - Publisher::Ref - reference to the object doing the signalling
        - boolean indicating the state of the publishing
          
	To disconnect, simply invoke the disconnect() method of the returned object.

        \param observer object to receive notifications

        \return object identifying the signal connection
    */
    SignalConnection connectToPublishedSignal(PublishedSignalProc observer)
        { return publishedSignal_.connect(observer); }

    /** Set or modify an entry in the service's published TXT record.

        \param key name of the value to add/change

        \param value value to store for the given key

	\param postNow if true, submit the change to the DNS server now

        \return true if successful, false otherwise
    */
    bool setTextData(const std::string& key, const std::string& value, bool postNow = true);

    /** Post any changes to the entry's TXT record.
     */
    bool postTextData();

    /** Initiate the publishing of information regarding this service. Note that successful publishing is not
        indicated by a good result from this routine, but rather when the method published() is invoked, with a
        true state value. To stop publishing, invoke Transaction::stop().

        \param name name of this service

        \param noRename if true, fail to publish if another service with the same name exists. Otherwise, let
        Zeroconf deconflict names, giving us a new one in the PublishedSignalProc callback.

        \return true if successful, false otherwise
    */
    bool publish(const std::string& name, bool noRename);

    /** Obtain whether we are published or not.

        \return true if so
    */
    bool isPublished() const
	{ return isPublished_ && isRunning(); }

protected:

    /** Constructor. Restricted to derived classes and to the Make() factory method.
     */
    Publisher(Monitor* monitor);

    /** Overridable hook method called to process a DNSServiceRegister response from the DNSSD server. This
        implementation sets the isPublished_ attribute and then invokes publishedService().

        \param err error code from the DNSSD server

        \param name the name that was actually published

        \param type the type that was published

        \param domain the domain under which everything was published
    */
    virtual void processResponse(DNSServiceErrorType err, const char* name, const char* type, const char* domain);

    /** Overridable hook method called to notify all PublishedSignal observers that our publishing state has
        changed.
        
        \param published true if currently publishing
    */
    virtual void notifyObservers(bool published);

private:
    
    /** Push any changes to our TXT record to the DNSSD daemon.
        
        \return true if successful
    */
    bool updateTextRecord();

    /** Override of Transaction method. Resets isPublished_ flag to false.
     */
    void serviceStopping();

    PublishedSignal publishedSignal_; ///< Signal for published state changes
    StringMap textData_;	///< Current TXT fields
    std::string name_;		///< Name we are publishing
    std::string type_;		///< Type we are publishing
    std::string domain_;	///< Domain we are publishing under
    uint32_t interface_;
    uint16_t port_;		///< Port we are publishing
    bool isPublished_;		///< True if we are currently publishing

    /** DNSServiceRegister callback. Invoked when the DNSSD server has completed a DNSServiceRegister request to
        publish our information.

        \param ref reference to the publish request

        \param flags request flags (ignored)

        \param err result of the call

        \param name actual name that was published

        \param type actual type that was published

        \param domain domain published under

        \param context pointer to the Publish object that called
        DNSServiceRegister.
    */
    static void RegisterCallback(DNSServiceRef ref, DNSServiceFlags flags, DNSServiceErrorType err,
                                 const char* name, const char* type, const char* domain, void* context);
};

} // end of namespace Zeroconf
} // end of namespace SideCar

/** \file
 */

#endif
