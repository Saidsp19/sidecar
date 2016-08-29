#include <sstream>

#include "ace/Reactor.h"

#include "Logger/Log.h"
#include "Utils/IO.h"
#include "Zeroconf/ACEMonitor.h"
#include "Zeroconf/Publisher.h"

#include "MessageManager.h"
#include "MulticastDataPublisher.h"

using namespace SideCar::IO;
using namespace SideCar::Zeroconf;

Logger::Log&
MulticastDataPublisher::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.MulticastDataPublisher");
    return log_;
}

MulticastDataPublisher::Ref
MulticastDataPublisher::Make()
{
    Ref ref(new MulticastDataPublisher);
    return ref;
}

MulticastDataPublisher::MulticastDataPublisher()
    : Super(), writer_(), heartBeatReader_(), timer_(-1), heartBeats_()
{
    ;
}

MulticastDataPublisher::~MulticastDataPublisher()
{
    ;
}

void
MulticastDataPublisher::setServiceName(const std::string& serviceName)
{
    Super::setServiceName(serviceName);
    std::ostringstream os;
    os << serviceName << " PUB";
    setTaskName(os.str());
}

bool
MulticastDataPublisher::openAndInit(const std::string& key, const std::string& serviceName,
                                    const std::string& multicastAddress, long threadFlags, long threadPriority)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " serviceName: " << serviceName
	    << " address: " << multicastAddress
	    << " threadFlags: " << std::hex << threadFlags << std::dec
	    << " threadPriority: " << threadPriority << std::endl;

    if (! reactor()) reactor(ACE_Reactor::instance());

    threadFlags_ = threadFlags;
    threadPriority_ = threadPriority;
    setMetaTypeInfoKeyName(key);
    setTransport("multicast");

    // Disallow any data into our processing queue. Activate it later when we've successfully published our
    // connection information, just before we start our writer thread.
    //
    msg_queue()->deactivate();

    // Working directly with the ACE datagram (UDP) object -- this perhasps should be refactored into the
    // UDPWriter class itself as an open() call.
    //
    ACE_SOCK_Dgram& device(writer_.getDevice());

    // Open the writer device to use to send out data. For opening, we use the "any" address with a zero port.
    // The system will assign us a port number to use.
    //
    ACE_INET_Addr address(u_short(0), "0.0.0.0", AF_INET);
    LOGDEBUG << "opening UDP socket with address "
	     << Utils::INETAddrToString(address) << std::endl;
    if (device.open(address, AF_INET, 0, 1) == -1) {
	LOGERROR << "failed to open multicast writer at " << multicastAddress << std::endl;
	return false;
    }

    LOGDEBUG << "opened socket " << writer_.getDevice().get_handle() << std::endl;

    // Set the time-to-live to a non-zero value so we don't break the MIT/LL network.
    //
    unsigned char ttl = 10;
    if (device.set_option(IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
	LOGERROR << "failed to set TTL for " << multicastAddress << std::endl;
	return false;
    }

    // Fetch the port assigned to us by the system.
    //
    device.get_local_addr(address);
    uint16_t port = address.get_port_number();
    LOGDEBUG << "socket device address: " << Utils::INETAddrToString(address) << std::endl;

    // Build our multicast address now that we know our port number. Tell the writer where to send the UDP
    // messages.
    //
    address.set(port, multicastAddress.c_str(), 1, AF_INET);
    LOGDEBUG << "mcast address: " << Utils::INETAddrToString(address) << std::endl;
    writer_.setRemoteAddress(address);

    // Set our Zeroconf publisher connection info.
    //
    std::string serviceType(MakeZeroconfType(key));
    LOGDEBUG << "serviceType: " << serviceType << std::endl;
    setType(serviceType);	// Our unique service ID
    setPort(port);		// Our unique port number
    setHost(multicastAddress); // Our assigned multicast address 

    // Create our heart-beat reader. First, open a new UDP socket so we can get its system-assigned port value.
    //
    address.set(uint16_t(0));
    if (heartBeatReader_.open(address) == -1) {
	LOGERROR << "failed to create heart-beat UDP reader" << std::endl;
	return false;
    }

    // Install ourselves in the global ACE_Reactor so that we receive notification when there is data available
    // on the heart-beat port.
    //
    if (reactor()->register_handler(heartBeatReader_.get_handle(), this, ACE_Event_Handler::READ_MASK) == -1) {
	LOGERROR << "failed to register heart-beat input handler" << std::endl;
	return false;
    }

    // Fetch the port assigned to us by the system.
    //
    heartBeatReader_.get_local_addr(address);
    LOGDEBUG << "heartbeat address: " << Utils::INETAddrToString(address) << std::endl;

    // Convert port number to text so we can store in the published TEXT record.
    //
    std::ostringstream os;
    os << address.get_port_number();
    getConnectionPublisher()->setTextData("HeartBeatPort", os.str());

    // Now we are ready to publish our connection information.
    //
    if (! publish(serviceName)) {
	LOGERROR << "failed to publish connection with name '" << serviceName << " type: " << serviceType
                 << std::endl;
	return false;
    }

    // Create a timer to periodically check for expired connections.
    //
    const ACE_Time_Value delay(5);
    const ACE_Time_Value repeat(5);
    timer_ = reactor()->schedule_timer(this, &timer_, delay, repeat);

    return true;
}

void
MulticastDataPublisher::publishSucceeded()
{
    Logger::ProcLog log("publishSucceeded", Log());
    LOGINFO << std::endl;

    // We are now ready to process incoming messages. Enable our message queue and start a thread running the
    // svc() method to process the queue.
    //
    msg_queue()->activate();
    activate(threadFlags_, 1, 0, threadPriority_);

    Super::publishSucceeded();
}

int
MulticastDataPublisher::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << "flags: " << flags << std::endl;

    if (flags) {

	// Disable the message queue. This will signal our writer thread to exit. Be sure and wait until it
	// does.
	//
	if (! msg_queue()->deactivated()) {
	    msg_queue()->deactivate();
	    wait();
	}

	// Now shutdown the writer and heart-beat UDP reader.
	//
	writer_.getDevice().close();
	reactor()->remove_handler(heartBeatReader_.get_handle(),
                                  ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL);
	heartBeatReader_.close();

	// Stop the heart-beat timer
	//
	if (timer_ != -1) {
	    reactor()->cancel_timer(timer_);
	    timer_ = -1;
	}

	heartBeats_.clear();
    }

    return Super::close(flags);
}

int
MulticastDataPublisher::handle_input(ACE_HANDLE handle)
{
    static Logger::ProcLog log("handle_input", Log());
    LOGINFO << std::endl;

    // Fetch the heart-beat message. Should be 'HI' or 'BYE'. Actually, this whole routine should be refactored
    // into another class.
    //
    ACE_TCHAR buffer[16];
    ACE_INET_Addr address;
    ssize_t count = heartBeatReader_.recv(buffer, sizeof(buffer), address);
    if (count < 1) {
	LOGERROR << "failed recv - " << errno << ' ' << strerror(errno) << std::endl;
	return 0;
    }

    std::string msg(buffer);
    LOGDEBUG << "msg: " << msg << std::endl;

    // Convert the client address into a string to be used as a key into the HeartBeatMap container.
    //
    std::string key(Utils::INETAddrToString(address));
    if (key.size() == 0) {
	LOGERROR << "failed INETAddrToString" << std::endl;
	return 0;
    }

    LOGDEBUG << "key: " << key << std::endl;
    if (msg == "HI") {

	// Update the timestamp for the client address. This also works for first-time additions.
	//
	heartBeats_[key] = Time::TimeStamp::Now();
    }
    else if (msg == "BYE") {

	// Remove the entry for the client address.
	//
	HeartBeatMap::iterator pos = heartBeats_.find(key);
	if (pos != heartBeats_.end()) heartBeats_.erase(pos);
    }

    updateUsingDataValue();

    LOGDEBUG << "heartBeats_.empty(): " << heartBeats_.empty() << std::endl;
    return 0;
}

int
MulticastDataPublisher::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    static Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << std::endl;

    if (arg != &timer_) return Super::handle_timeout(duration, arg);

    if (heartBeats_.empty()) return 0;

    // Calculate a time that is 60 seconds ago. If a timestamp is older than that, assume the connection is
    // gone.
    //
    Time::TimeStamp limit = Time::TimeStamp::Now();
    limit -= Time::TimeStamp(60, 0);

    HeartBeatMap::iterator pos = heartBeats_.begin();
    HeartBeatMap::iterator end = heartBeats_.end();
    while (pos != end) {
	if (pos->second > limit) {
	    ++pos;
	}
	else {

	    // !!! Erasing an item from std::map affects only affects iterators pointing to the item. So, we
	    // !!! need to advance our loop iterator before peforming the erase.
	    //
	    HeartBeatMap::iterator tmp = pos++;
	    LOGDEBUG << "forgetting " << pos->first << std::endl;
	    heartBeats_.erase(tmp);
	}
    }

    updateUsingDataValue();

    return 0;
}

bool
MulticastDataPublisher::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    if (! isUsingData()) {
	data->release();
	return true;
    }

    return putq(data, timeout) != -1;
}

int
MulticastDataPublisher::svc()
{
    static Logger::ProcLog log("svc", Log());
    LOGINFO << "started" << std::endl;

    ACE_Message_Block* data;
    while (getq(data) != -1) {
	MessageManager mgr(data);
	if (! writer_.write(mgr)) {
	    LOGERROR << "failed to send the message" << std::endl;
	}
    }

    return 0;
}

bool
MulticastDataPublisher::calculateUsingDataValue() const
{
    return ! heartBeats_.empty() || Super::calculateUsingDataValue();
}
