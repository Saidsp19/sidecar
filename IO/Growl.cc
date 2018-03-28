#include <algorithm>

#include "ace/INET_Addr.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Bcast.h"

#include <iostream>

#include "Utils/MD5.h"

#include "Growl.h"

using namespace SideCar::IO;

enum { kProtocolVersion = 1, kTypeRegistration = 0, kTypeNotification = 1, kPort = 9887 };

/** Utility class that holds information about a specific Growler destination.
 */
struct Growler::Destination {
    Destination(const std::string& name) : name_(name), address_(), emitRegistration_(true)
    {
        address_.set(kPort, name.c_str(), 1, AF_INET);
    }

    std::string name_;      ///< Host name running the Growler application
    ACE_INET_Addr address_; ///< IP address of the host
    bool emitRegistration_; ///< True if need to register
};

/** Utility class that can build Growl payload packets.
 */
struct PayloadBuffer {
    using PayloadType = std::vector<uint8_t>;

    PayloadBuffer(int type, const std::string& password);

    PayloadBuffer& operator<<(unsigned char value);

    PayloadBuffer& operator<<(bool value) { return operator<<(static_cast<unsigned char>(value ? 1 : 0)); }

    PayloadBuffer& operator<<(char value) { return operator<<(static_cast<unsigned char>(value)); }

    PayloadBuffer& operator<<(uint16_t value);

    PayloadBuffer& operator<<(int16_t value) { return operator<<(static_cast<uint16_t>(value)); }

    PayloadBuffer& operator<<(uint32_t value) { return operator<<(static_cast<uint16_t>(value & 0xFFFF)); }

    PayloadBuffer& operator<<(int32_t value) { return operator<<(static_cast<uint16_t>(value & 0xFFFF)); }

    PayloadBuffer& operator<<(size_t value) { return operator<<(static_cast<uint16_t>(value & 0xFFFF)); }

    PayloadBuffer& operator<<(const std::string& value);

    std::vector<uint8_t> getPayload();

    std::vector<uint8_t> buffer_;
    std::string password_;
};

PayloadBuffer::PayloadBuffer(int type, const std::string& password) : buffer_(), password_(password)
{
    // Build the header: one byte for the protocol and one for the payload type (registration or notification)
    //
    buffer_.push_back(static_cast<uint8_t>(kProtocolVersion));
    buffer_.push_back(static_cast<uint8_t>(type));
}

PayloadBuffer&
PayloadBuffer::operator<<(unsigned char value)
{
    // Add a byte
    //
    buffer_.push_back(static_cast<uint8_t>(value));
    return *this;
}

PayloadBuffer&
PayloadBuffer::operator<<(uint16_t value)
{
    // Add a short as two bytes, always in network byte order (big-endian)
    //
    value = htons(value);
    buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer_.push_back(static_cast<uint8_t>(value >> 8));
    return *this;
}

PayloadBuffer&
PayloadBuffer::operator<<(const std::string& value)
{
    // Add string characters. NOTE: the string size is not added here, since the Growl specification has it in
    // different places depenending on what string it is.
    //
    for (size_t index = 0; index < value.size(); ++index) { buffer_.push_back(static_cast<uint8_t>(value[index])); }
    return *this;
}

PayloadBuffer::PayloadType
PayloadBuffer::getPayload()
{
    // Obtain the payload byte stream. Calculate an MD5 checksum for it and add before returning.
    //
    Utils::MD5 md5;
    md5.add(&buffer_[0], buffer_.size());
    if (password_.size()) md5.add(password_);

    const Utils::MD5::DigestType& digest(md5.getDigest());
    for (int index = 0; index < Utils::MD5::kDigestSize; ++index) buffer_.push_back(digest[index]);

    return buffer_;
}

Growler::Growler(const std::string& name, const std::string& password) :
    name_(name), password_(password), notifications_(), defaults_(), destinations_(), socket_(),
    usingBroadcasting_(false)
{
    useBroadcasting(true);
}

Growler::~Growler()
{
    ;
}

void
Growler::addHost(const std::string& host)
{
    // See if the host being added is for broadcasting
    //
    if (host == "255.255.255.255" || host == "<broadcast>") {
        useBroadcasting(true);
    } else {
        // See if the host already exists in our list of destinations.
        //
        for (DestinationList::const_iterator pos = destinations_.begin(); pos != destinations_.end(); ++pos) {
            if ((*pos)->name_ == host) return;
        }

        // Switch from broadcasting mode if necessary and add new destination
        //
        useBroadcasting(false);
        boost::shared_ptr<Destination> ref(new Destination(host));
        destinations_.push_back(ref);
    }
}

void
Growler::useBroadcasting(bool value)
{
    if (value == usingBroadcasting_ && socket_) return;

    usingBroadcasting_ = value;

    // Broadcasting and direct UDP use different delivery objects, so we always close and deallocate any
    // previous socket.
    //
    if (socket_) {
        socket_->close();
        socket_.reset();
    }

    // When switching modes, forget any previous destinations.
    //
    destinations_.clear();

    if (usingBroadcasting_) {
        // Create a delivery object for broadcast UDP messages, and add a broadcast IP host to use.
        //
        socket_.reset(new ACE_SOCK_Dgram_Bcast(ACE_INET_Addr(uint16_t(0))));
        boost::shared_ptr<Destination> ref(new Destination("255.255.255.255"));
        destinations_.push_back(ref);
    } else {
        // Create a normal UDP delivery object. User must still add one or more hosts.
        //
        socket_.reset(new ACE_SOCK_Dgram(ACE_INET_Addr(uint16_t(0))));
    }
}

void
Growler::addNotification(const std::string& name, bool enabled)
{
    if (enabled) defaults_.push_back(notifications_.size());
    notifications_.push_back(name);

    // Flag all destinations to reregister before sending the next notification
    //
    for (DestinationList::const_iterator pos = destinations_.begin(); pos != destinations_.end(); ++pos) {
        (*pos)->emitRegistration_ = true;
    }
}

bool
Growler::emitRegistration(boost::shared_ptr<Destination> destination)
{
    // Build a registration payload.
    //
    PayloadBuffer buffer(kTypeRegistration, password_);
    buffer << name_.size() << uint8_t(notifications_.size()) << uint8_t(defaults_.size()) << name_;

    // Add the names of the notifications.
    //
    for (size_t index = 0; index < notifications_.size(); ++index)
        buffer << notifications_[index].size() << notifications_[index];

    // Add the indices of those notifications that are enabled
    //
    for (size_t index = 0; index < defaults_.size(); ++index) buffer << defaults_[index];

    // Emit the payload to a specific destination.
    //
    PayloadBuffer::PayloadType payload(buffer.getPayload());
    bool rc = socket_->send(&payload[0], payload.size(), destination->address_, 0) == ssize_t(payload.size());
    if (rc) destination->emitRegistration_ = false;

    return rc;
}

bool
Growler::notify(const std::string& notification, const std::string& title, const std::string& description, int priority,
                bool sticky)
{
    // The notification name must exist in our registry.
    //
    if (std::find(notifications_.begin(), notifications_.end(), notification) == notifications_.end()) return false;

    // Build a notification payload.
    //
    PayloadBuffer buffer(kTypeNotification, password_);
    unsigned short flags = (priority & 0x07) * 2;
    if (priority < 0) flags |= 0x08;
    if (sticky) flags |= 1;

    buffer << flags << notification.size() << title.size() << description.size() << name_.size() << notification
           << title << description << name_;

    // Emit the payload to all destinations. We will return false if we failed to send to at least one
    // destination.
    //
    PayloadBuffer::PayloadType payload(buffer.getPayload());

    for (DestinationList::iterator pos = destinations_.begin(); pos != destinations_.end();) {
        bool ok = true;

        if ((*pos)->emitRegistration_) ok = emitRegistration(*pos);

        if (ok) socket_->send(&payload[0], payload.size(), (*pos)->address_, 0);

        ++pos;
    }

    return true;
}
