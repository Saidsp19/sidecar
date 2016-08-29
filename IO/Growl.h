#ifndef SIDECAR_IO_GROWL_H // -*- C++ -*-
#define SIDECAR_IO_GROWL_H

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"

#include <list>
#include <string>
#include <vector>

class ACE_SOCK_Dgram;

namespace SideCar {
namespace IO {

/** C++ interface to the Growl notification display service for Mac OS X (and other systemsw via various ports).
    Growl is sort of like Twitter for applications: an application emits UDP messages to one or more Growl
    servers. This implementation also supports UDP broadcasting.

    To use, create a new Growler instance, add one one or more notification names, and when desired invoked
    notify() to emit send out a Growl UDP message.
*/
class Growler
{
public:

    /** Constructor for a new Growler object

        \param name the name to emit under

        \param password the optional password to submit. If used, this must match the value set for the Growl
        application
    */
    Growler(const std::string& name, const std::string& password = "");

    /** Destructor. Releases any notification names and destinations.
     */
    virtual ~Growler();

    /** Add a new destination.

        \param host the name or IP address of the host to contact
    */
    void addHost(const std::string& host);

    /** Remove an existing destination.

        \param host the name or IP address of the host to remove
    */
    void removeHost(const std::string& host);

    /** Enable (or disable) UDP broadcasting for Growl notifications. Note that calling addHost() will
        automatically disable broadcasting.

        \param value true if enabled.
    */
    void useBroadcasting(bool value = true);

    /** Add a new notification name to support. Future notify() calls must use a notification name previously
        registered with addNotification().

        \param name the name to register

        \param enabled whether the notification is currently enabled
    */
    void addNotification(const std::string& name, bool enabled = true);

    /** Emit a notification to registered hosts or broadcasted to the network.

        \param notification the registered notification to use

        \param title the summary of the message

        \param description the detailed contents of the message

        \param priority optional priority for the message

        \param sticky if true, the message will not fade away

        \return true if successful
    */
    bool notify(const std::string& notification, const std::string& title, const std::string& description,
                int priority = 0, bool sticky = false);

private:

    struct Destination;

    bool emitRegistration(boost::shared_ptr<Destination> destination);

    std::string name_;
    std::string password_;
    std::vector<std::string> notifications_;
    std::vector<unsigned char> defaults_;

    using DestinationList = std::list<boost::shared_ptr<Destination> >;
    DestinationList destinations_;

    boost::scoped_ptr<ACE_SOCK_Dgram> socket_;
    bool usingBroadcasting_;
};

} // end namespace IO
} // end namespace SideCar

#endif
