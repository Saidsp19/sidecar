#ifndef SIDECAR_ZEROCONF_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_DOXYGEN_H

/** \page zeroconf Zeroconf Overview

    Although the SideCar system relies on an XML configuration file to define processing stream and startup
    values for algorithms, socket connectivity information such as port and host name for data publishers and
    subscribers is not stored there. Instead, data publishers and subscribers use the <a
    href="http://www.zeroconf.org/">Zero Configuration Networking</a> (Zeroconf) API and a <a
    href="http://www.multicastdns.org/">Multicast DNS</a> (mDNS) server running on each of the hosts in the
    SideCar LAN to share connection information. The discovery process is very simple. First, a data publisher
    publishes information to the Zeroconfig system using a SideCar::Zeroconf::Publisher instance. Published
    information includes the host name and port where the service is running. For a data subscriber, it uses a
    SideCar::Zeroconf::Browser instance to locate data publishers of a particular data type. When it finds one,
    it asks the mDNS server to resolve the publisher's connection information, and when the subscriber receives
    the response, it then initiates a connection to the data publisher.

    GUI applications use the same mechanism to discover data publishers, but they can offer the user with a list
    of active publishers, and the list is dynamically updated as publishers become available or disappear. The
    SideCar::GUI::PPIDisplay and SideCar::GUI::AScope applications both rely on this Zeroconf service discovery
    to offer their users valid, up-to-date channel connection options.
*/

namespace SideCar {

/** Namespace for entities that interface with Zeroconf library.
 */
namespace Zeroconf {
}

} // end namespace SideCar

#endif
