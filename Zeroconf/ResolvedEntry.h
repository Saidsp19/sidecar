#ifndef SIDECAR_ZEROCONF_RESOLVEDENTRY_H // -*- C++ -*-
#define SIDECAR_ZEROCONF_RESOLVEDENTRY_H

#include <map>
#include <string>

namespace SideCar {
namespace Zeroconf {

/** Contains information obtained from resolving a ServiceEntry object. In the Zeroconf world, services found by
    a Browser do not reveal concrete connection information, they only contain a name and a type. Client
    applications of a Browser must ask a ServiceEntry for its connection information via
    ServiceEntry::resolve(). This queries the multicast DNS server running locally, which returns the host/port
    connection pair.
*/
class ResolvedEntry {
public:
    using StringMap = std::map<std::string, std::string>;

    /** Constructor. Fills in a new ResolvedEntry object with information from the mdns server.

        \param fullName the full service name

        \param host the host identifier where the service is running

        \param port the port of the host where the service is running

        \param textSize the number of bytes in the entry's TXT record

        \param textData a pointer to the first byte of TXT record data
    */
    ResolvedEntry(const std::string& fullName, const std::string& host, uint16_t port, uint16_t textSize,
                  const unsigned char* textData);

    /** Obtain the full name of the service

        \return full name
    */
    const std::string& getFullName() const { return fullName_; }

    const std::string& getNativeHost() const { return nativeHost_; }

    /** Obtain the host where the service is running

        \return host identifier
    */
    const std::string& getHost() const { return host_; }

    /** Obtain the port where the service is running

        \return port id
    */
    uint16_t getPort() const { return port_; }

    /** Obtain the TXT record of the entry as a key/value map.

        \return read-only string map
    */
    const StringMap& getTextEntries() const { return textEntries_; }

    /** Determine if a key exist in the TXT record

        \param key the value to search for

        \param value if non-zero, storage for the value found for the given key

        \return true if key found, false otherwise
    */
    bool hasTextEntry(const std::string& key, std::string* value = 0) const;

    /** Obtain the value in a TXT record for a given key.

        \param key the value to search for

        \return true if key found, false otherwise
    */
    std::string getTextEntry(const std::string& key) const;

private:
    /** Recreate the textEntries_ attribute with values from a resolved callback.

        \param textSize number of bytes in the TXT record

        \param textData pointer to the first byte in the TXT record
    */
    void updateTextEntries(uint16_t textSize, const unsigned char* textData);

    std::string fullName_;
    std::string nativeHost_;
    std::string host_;
    uint16_t port_;
    StringMap textEntries_;
};

} // end namespace Zeroconf
} // end namespace SideCar

/** \file
 */

#endif
