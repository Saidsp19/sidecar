// #include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Dgram.h"
#include "ace/Task.h"

#include "Logger/Log.h"
#include "Utils/Format.h"
#include "Utils/IO.h"
#include "Utils/Utils.h"
#include "XMLRPC/XmlRpcValue.h"
#include "Zeroconf/ACEMonitor.h"
#include "Zeroconf/Browser.h"
#include "Zeroconf/ResolvedEntry.h"
#include "Zeroconf/ServiceEntry.h"

#include "StateEmitter.h"

namespace SideCar {
namespace IO {

struct StateEmitter::Private : public ACE_Task<ACE_MT_SYNCH> {
    using Super = ACE_Task<ACE_MT_SYNCH>;

public:
    /** Connection information for a StatusCollector.
     */
    struct Destination {
        Destination(const Zeroconf::ServiceEntry::Ref& serviceEntry) :
            serviceEntry_(serviceEntry), address_(), failures_(0)
        {
        }
        Zeroconf::ServiceEntry::Ref serviceEntry_;
        ACE_INET_Addr address_;
        int failures_;
    };

    using DestinationVector = std::vector<Destination*>;
    using ServiceEntryVector = Zeroconf::Browser::ServiceEntryVector;

    Private();
    ~Private();
    bool openAndInit(const std::string& emitterName, long threadFlags, long priority);
    int close(u_long flags) override;
    void setState(const std::string& key, const std::string& value);
    void removeState(const std::string& key);
    void clearState();
    void publish();

    size_t findDestinationIndex(const Zeroconf::ServiceEntry::Ref& service);
    Destination* findDestination(const Zeroconf::ServiceEntry::Ref& service);
    ACE_HANDLE get_handle() const override { return socket_.get_handle(); }
    void bringUpToDate(Destination* destination) const;
    void sendTo(Destination* destination, const ACE_Message_Block* d) const;
    void foundSlot(const ServiceEntryVector& services);
    void lostSlot(const ServiceEntryVector& services);
    void resolvedSlot(const Zeroconf::ServiceEntry::Ref& service);
    int svc() override;

    ACE_SOCK_Dgram socket_;
    Zeroconf::Browser::Ref browser_;
    DestinationVector destinations_;
    XmlRpc::XmlRpcValue state_;
};

} // namespace IO
} // namespace SideCar

using namespace SideCar;
using namespace SideCar::IO;

Logger::Log&
StateEmitter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.StateEmitter");
    return log_;
}

StateEmitter::Private::Private() :
    Super(), socket_(ACE_INET_Addr(uint16_t(0))),
    browser_(Zeroconf::Browser::Make(Zeroconf::ACEMonitorFactory::Make(), GetZeroconfType())), destinations_(),
    state_(new XmlRpc::XmlRpcValue::ValueStruct)
{
    Logger::ProcLog log("Private", Log());

    // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers. Apparently,
    // !!! mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management. However, we still need
    // !!! this on Mac OS X (Darwin), and perhaps others.
    //
#ifdef darwin
    int value = 64 * 1024; // 64K IO buffer
    if (socket_.set_option(SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) == -1)
        LOGERROR << "failed to set SO_SNDBUF to " << value << std::endl;
#endif

    browser_->connectToFoundSignal([this](auto& v) { foundSlot(v); });
    browser_->connectToLostSignal([this](auto& v) { lostSlot(v); });
}

StateEmitter::Private::~Private()
{
    for (auto d : destinations_) delete d;
    destinations_.clear();
}

bool
StateEmitter::Private::openAndInit(const std::string& emitterName, long threadFlags, long priority)
{
    static Logger::ProcLog log("openAndInit", Log());
    LOGINFO << std::endl;

    state_["emitterName"] = emitterName;
    state_["state"] = new XmlRpc::XmlRpcValue::ValueStruct;

    if (!reactor()) reactor(ACE_Reactor::instance());

    // Start our browser to detect any state collectors.
    //
    if (!browser_->start()) {
        LOGFATAL << "failed to start browser - " << Utils::showErrno() << std::endl;
        return false;
    }

    // Start our emitter thread
    //
    if (activate(threadFlags, 1, 0, priority) == -1) {
        LOGFATAL << "failed to start emitter thread - " << Utils::showErrno() << std::endl;
        return false;
    }

    return true;
}

int
StateEmitter::Private::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << std::endl;

    Super::close(flags);

    if (flags && !msg_queue()->deactivated()) {
        // Signal the emitter thread to exit and wait for it to die.
        //
        msg_queue()->deactivate();
        wait();

        browser_->stop();

        if (socket_.get_handle() != ACE_INVALID_HANDLE) { socket_.close(); }
    }

    return 0;
}

void
StateEmitter::Private::setState(const std::string& key, const std::string& value)
{
    static Logger::ProcLog log("setState", Log());
    LOGINFO << key << ' ' << value << std::endl;
    state_["state"][key] = value;
}

void
StateEmitter::Private::removeState(const std::string& key)
{
    static Logger::ProcLog log("removeState", Log());
    LOGINFO << key << std::endl;
    XmlRpc::XmlRpcValue::ValueStruct& tmp(state_["state"]);
    tmp.erase(key);
}

void
StateEmitter::Private::clearState()
{
    static Logger::ProcLog log("clearState", Log());
    LOGINFO << std::endl;
    state_["state"] = new XmlRpc::XmlRpcValue::ValueStruct;
}

void
StateEmitter::Private::publish()
{
    static Logger::ProcLog log("publish", Log());
    LOGINFO << std::endl;

    // Create an XML message to publish.
    //
    std::string buffer(state_.toXml());
    ACE_Message_Block* data = new ACE_Message_Block(buffer.size(), ACE_Message_Block::MB_DATA);
    data->copy(buffer.c_str(), buffer.size());

    // Publish the message to our emitter.
    //
    if (putq(data) == -1) {
        LOGERROR << "failed to enqueue updated state" << std::endl;
        data->release();
    }
}

int
StateEmitter::Private::svc()
{
    Logger::ProcLog log("svc", Log());

    ACE_Message_Block* data = 0;
    Destination* dest = 0;

    // Fetch data blocks from our processing queue. Keep going until our queue returns an error because it has
    // been deactivated.
    //
    while (getq(data) != -1) {
        switch (data->msg_type()) {
        case ACE_Message_Block::MB_DATA:

            // Emit the current status to all active destinations.
            //
            for (auto d : destinations_) sendTo(d, data);
            break;

        case ACE_Message_Block::MB_START:

            // New Destination object to add. First make sure that it points to a unique location.
            //
            dest = reinterpret_cast<Destination*>(data->base());
            for (size_t index = 0; index < destinations_.size(); ++index) {
                if (destinations_[index]->serviceEntry_->getName() == dest->serviceEntry_->getName()) {
                    delete dest;
                    dest = 0;
                    break;
                }
            }

            if (dest) {
                destinations_.push_back(dest);
                bringUpToDate(dest);
            }
            break;

        case ACE_Message_Block::MB_STOP:

            // Destination object to remove. Scan the Destination vector for the entry to remove and then remove
            // it.
            //
            dest = reinterpret_cast<Destination*>(data->base());
            for (size_t index = 0; index < destinations_.size(); ++index) {
                if (destinations_[index] == dest) {
                    destinations_.erase(destinations_.begin() + index);
                    delete dest;
                    break;
                }
            }
            break;

        default: break;
        }

        data->release();
        data = 0;
    }

    return 0;
}

void
StateEmitter::Private::bringUpToDate(Destination* destination) const
{
    std::string buffer(state_.toXml());
    ACE_Message_Block* data = new ACE_Message_Block(buffer.size(), ACE_Message_Block::MB_DATA);
    data->copy(buffer.c_str(), buffer.size());
    sendTo(destination, data);
    data->release();
}

void
StateEmitter::Private::sendTo(Destination* destination, const ACE_Message_Block* data) const
{
    static Logger::ProcLog log("sendTo", Log());
    Zeroconf::ServiceEntry::Ref serviceEntry = destination->serviceEntry_;

    LOGINFO << serviceEntry->getName() << " port: " << destination->address_.get_port_number() << ' '
            << destination->address_.get_host_name() << std::endl;

    // If we've received more than 10 errors in a row, stop sending to this destination.
    //
    if (destination->failures_ > 10) return;

    // Attempt to send to the StatusCollector found at the Destination object.
    //
    ssize_t rc = socket_.send(data->rd_ptr(), data->size() - 1, destination->address_, 0);
    if (rc != ssize_t(data->size() - 1)) {
        ++destination->failures_;
        LOGERROR << serviceEntry->getName() << " failed send to " << Utils::INETAddrToString(destination->address_)
                 << " - buffer size: " << data->size() << " - " << Utils::showErrno() << std::endl;
    } else {
        destination->failures_ = 0;
    }
}

void
StateEmitter::Private::foundSlot(const ServiceEntryVector& services)
{
    static Logger::ProcLog log("foundSlot", Log());
    LOGINFO << services.size() << std::endl;

    // We resolve all new services, and only after it is resolved do we submit it to the emitter thread to add
    // it to the vector of Destination objects.
    //
    for (size_t index = 0; index < services.size(); ++index) {
        Zeroconf::ServiceEntry::Ref service(services[index]);
        service->connectToResolvedSignal([this](auto& v) { resolvedSlot(v); });
        service->resolve();
    }
}

void
StateEmitter::Private::resolvedSlot(const Zeroconf::ServiceEntry::Ref& service)
{
    static Logger::ProcLog log("resolvedSlot", Log());

    const Zeroconf::ResolvedEntry& resolvedEntry = service->getResolvedEntry();
    LOGINFO << "fullname: " << resolvedEntry.getFullName() << " host: " << resolvedEntry.getHost()
            << " port: " << resolvedEntry.getPort() << " interface: " << service->getInterface() << std::endl;

    std::string host(resolvedEntry.getHost());
    LOGDEBUG << "host: " << host << std::endl;

    ACE_INET_Addr address;
    if (address.set(resolvedEntry.getPort(), host.c_str(), 1, AF_INET) == -1) {
        LOGFATAL << "invalid address to use: " << host << '/' << resolvedEntry.getPort() << " - " << Utils::showErrno()
                 << std::endl;
        return;
    }

    // Create a new Destination object to represent the location of the remote StatusCollector. Encapsulate it
    // in an ACE_Message_Block and post it to the emitter thread.
    //
    Destination* dest = new Destination(service);
    dest->address_ = address;
    ACE_Message_Block* data =
        new ACE_Message_Block(sizeof(*dest), ACE_Message_Block::MB_START, 0, reinterpret_cast<char*>(dest));
    if (putq(data) == -1) {
        LOGERROR << "failed to enqueue new destination" << std::endl;
        data->release();
        delete dest;
    }
}

size_t
StateEmitter::Private::findDestinationIndex(const Zeroconf::ServiceEntry::Ref& service)
{
    // Locate the an existing Destination object that refers to a given Zeroconf service.
    //
    for (size_t index = 0; index < destinations_.size(); ++index) {
        Destination* destination = destinations_[index];
        Zeroconf::ServiceEntry::Ref held(destination->serviceEntry_);
        if (held->getName() == service->getName() && held->getDomain() == service->getDomain()) return index;
    }

    // Not found. Return the size of the Destination vector as a failure flag.
    //
    return destinations_.size();
}

void
StateEmitter::Private::lostSlot(const ServiceEntryVector& services)
{
    static Logger::ProcLog log("lostSlot", Log());
    LOGINFO << services.size() << std::endl;

    // Locate the Destination object we will remove, but let the emitter thread do the removal.
    //
    for (size_t index = 0; index < services.size(); ++index) {
        Zeroconf::ServiceEntry::Ref service(services[index]);
        size_t serviceIndex = findDestinationIndex(service);
        if (serviceIndex == destinations_.size()) {
            LOGWARNING << "registration for " << service->getName() << " not found" << std::endl;
            continue;
        }

        Destination* dest = destinations_[serviceIndex];

        // Encapsulate the Destination object to remove in an ACE_Message_Block and post to the emitter thread.
        //
        ACE_Message_Block* data =
            new ACE_Message_Block(sizeof(dest), ACE_Message_Block::MB_STOP, 0, reinterpret_cast<char*>(dest));
        if (putq(data) == -1) {
            LOGERROR << "failed to enqueue remove destination" << std::endl;
            data->release();
        }
    }
}

StateEmitter::StateEmitter() : p_(new Private)
{
    ;
}

StateEmitter::~StateEmitter()
{
    close();
}

bool
StateEmitter::open(const std::string& emitterName, long threadFlags, long priority)
{
    return p_->openAndInit(emitterName, threadFlags, priority);
}

void
StateEmitter::close()
{
    p_->close(1);
}

void
StateEmitter::setState(const std::string& key, const std::string& value)
{
    p_->setState(key, value);
}

void
StateEmitter::removeState(const std::string& key)
{
    p_->removeState(key);
}

void
StateEmitter::publish()
{
    p_->publish();
}
