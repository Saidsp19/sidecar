#include <cstdlib>
#include <string>
#include <vector>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Dgram.h"
#include "ace/Task.h"

#include "Logger/Log.h"
#include "Utils/Format.h"
#include "Utils/OrderedIndex.h"
#include "Utils/Utils.h"
#include "XMLRPC/XmlRpcValue.h"
#include "Zeroconf/ACEMonitor.h"
#include "Zeroconf/Browser.h"
#include "Zeroconf/ResolvedEntry.h"
#include "Zeroconf/ServiceEntry.h"

#include "StatusEmitterBase.h"
#include "Task.h"

namespace SideCar {
namespace IO {

/** Internal class used by StatusEmitterBase to do most of the work. Handles discovery of StatusCollector
    entities via Zeroconf::Browser, and transmission of status information to the active StatusCollector
    entities.
*/
struct StatusEmitterBase::Emitter : public ACE_Task<ACE_MT_SYNCH>
{
    using Super = ACE_Task<ACE_MT_SYNCH>;
public:

    /** Connection information for a StatusCollector.
     */
    struct Destination
    {
	Destination(const Zeroconf::ServiceEntry::Ref& serviceEntry)
            : serviceEntry_(serviceEntry), address_(), failures_(0) {}
	Zeroconf::ServiceEntry::Ref serviceEntry_;
	ACE_INET_Addr address_;
	int failures_;
    };

    using DestinationVector = std::vector<Destination*>;
    using ServiceEntryVector = Zeroconf::Browser::ServiceEntryVector;

    Emitter(StatusEmitterBase& owner);
    ~Emitter();

    size_t findDestinationIndex(const Zeroconf::ServiceEntry::Ref& service);
    Destination* findDestination(const Zeroconf::ServiceEntry::Ref& service);

    bool openAndInit(long threadFlags, long priority);
    int close(u_long flags);
    ACE_HANDLE get_handle() const { return socket_.get_handle(); }

    int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);

    int handle_timeout(const ACE_Time_Value& now, const void* context);

    void emitStatus();
    void startTimer();
    void stopTimer();
    void sendTo(Destination* destination) const;

    void foundSlot(const ServiceEntryVector& services);
    void lostSlot(const ServiceEntryVector& services);
    void resolvedSlot(const Zeroconf::ServiceEntry::Ref& service);

    bool isEmitting() const { return timerId_ != -1; }

    int svc();

    StatusEmitterBase& owner_;
    ACE_SOCK_Dgram socket_;
    Zeroconf::Browser::Ref browser_;
    DestinationVector destinations_;
    size_t destinationCount_;
    long timerId_;
    ACE_Message_Block* current_;
};

}} 				// end namespace SideCar::IO

using namespace SideCar;
using namespace SideCar::IO;

Logger::Log&
StatusEmitterBase::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.StatusEmitterBase");
    return log_;
}

StatusEmitterBase::Emitter::Emitter(StatusEmitterBase& owner)
    : Super(), owner_(owner), socket_(ACE_INET_Addr(uint16_t(0))),
      browser_(Zeroconf::Browser::Make(Zeroconf::ACEMonitorFactory::Make(), owner.collectorType_)),
      destinations_(), destinationCount_(0), timerId_(-1), current_(0)
{
    Logger::ProcLog log("StatusEmitterBase", Log());

    // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers. Apparently,
    // !!! mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management. However, we still need
    // !!! this on Mac OS X (Darwin), and perhaps others.
    //
#ifdef darwin
    int value = 64 * 1024;	// 64K IO buffer
    if (socket_.set_option(SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) == -1) {
	LOGERROR << "failed to set SO_SNDBUF to " << value << std::endl;
    }
#endif

    browser_->connectToFoundSignal([this](auto& v){foundSlot(v);});
    browser_->connectToLostSignal([this](auto& v){lostSlot(v);});
}

StatusEmitterBase::Emitter::~Emitter()
{
    std::for_each(destinations_.begin(), destinations_.end(), [](auto v){delete v;});
    destinations_.clear();
}

bool
StatusEmitterBase::Emitter::openAndInit(long threadFlags, long priority)
{
    static Logger::ProcLog log("openAndInit", Log());
    LOGINFO << std::endl;

    if (! reactor()) reactor(ACE_Reactor::instance());

    // Register ourselves to receive periodic calls to handle_timeout(). The timer is started inside the
    // startTimer() method.
    //
    if (reactor()->register_handler(this, TIMER_MASK) == -1) {
	LOGFATAL << "failed to register handler - " << Utils::showErrno() << std::endl;
	return false;
    }

    // Start our browser to detect any status collectors.
    //
    if (! browser_->start()) {
	LOGFATAL << "failed to start browser - " << Utils::showErrno() << std::endl;
	return false;
    }

    // Start our emitter thread
    //
    if (activate(threadFlags , 1, 0, priority) == -1) {
	LOGFATAL << "failed to start emitter thread - " << Utils::showErrno() << std::endl;
	return false;
    }

    return true;
}

int
StatusEmitterBase::Emitter::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << std::endl;

    Super::close(flags);

    if (flags && ! msg_queue()->deactivated()) {

	// Signal the emitter thread to exit and wait for it to die.
	//
	msg_queue()->deactivate();
	wait();

	// NOTE: I don't think we need READ_MASK here.
	//
	reactor()->remove_handler(this, READ_MASK | TIMER_MASK | DONT_CALL);
	if (timerId_ != -1) reactor()->cancel_timer(timerId_);
	if (socket_.get_handle() != ACE_INVALID_HANDLE) {
	    socket_.close();
	    browser_->stop();
	}
    }

    return 0;
}

void
StatusEmitterBase::Emitter::emitStatus()
{
    static Logger::ProcLog log("emitStatus", Log());

    // Generate a new XML status and post it to our emitter thread.
    //
    XmlRpc::XmlRpcValue status;
    owner_.fillStatus(status);
    std::string buffer(status.toXml());

    ACE_Message_Block* data = new ACE_Message_Block(buffer.size(), ACE_Message_Block::MB_DATA);
    data->copy(buffer.c_str(), buffer.size());
    if (putq(data) == -1) {
	LOGERROR << "failed to post status update to emitter queue" << std::endl;
        data->release();
    }
}

int
StatusEmitterBase::Emitter::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
{
    if (close_mask != ACE_Event_Handler::TIMER_MASK) {
	return close(1);
    }
    return 0;
}

int
StatusEmitterBase::Emitter::handle_timeout(const ACE_Time_Value& now, const void* context)
{
    static Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << std::endl;

    // Post a status update to the emitter queue.
    //
    emitStatus();
    return 0;
}

int
StatusEmitterBase::Emitter::svc()
{
    Logger::ProcLog log("svc", Log());
    ACE_Message_Block* data = nullptr;
    Destination* dest = nullptr;

    // Fetch data blocks from our processing queue. Keep going until our queue returns an error because it has
    // been deactivated.
    //
    while (getq(data) != -1) {

	switch(data->msg_type()) {
	case ACE_Message_Block::MB_DATA:

	    // Post status update to the all Destination objects.
	    //
	    if (current_) current_->release();
	    current_ = data;

	    // Emit the current status to all active destinations.
	    //
	    std::for_each(destinations_.begin(), destinations_.end(), [this](auto d){sendTo(d);});

	    // NOTE: don't call data->release() since we are caching it in current_.
	    //
	    break;

	case ACE_Message_Block::MB_START:

	    // New Destination object to add. First make sure that it points to a unique location.
	    //
	    dest = reinterpret_cast<Destination*>(data->base());
	    for (size_t index = 0; index < destinations_.size(); ++index) {
		if (destinations_[index]->serviceEntry_->getName() ==
                    dest->serviceEntry_->getName()) {

                    // Found existing entry -- ignore this new one.
                    //
		    delete dest;
		    dest = 0;
		    break;
		}
	    }

	    if (dest) {
		destinations_.push_back(dest);
		if (current_) sendTo(dest);
	    }

	    data->release();
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
	    data->release();
	    break;

	default:
            data->release();
	    break;
	}

        data = nullptr;
    }

    return 0;
}

void
StatusEmitterBase::Emitter::sendTo(Destination* destination) const
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
    ssize_t rc = socket_.send(current_->rd_ptr(), current_->size() - 1, destination->address_, 0);
    if (rc != ssize_t(current_->size() - 1)) {
	++destination->failures_;
	LOGERROR << serviceEntry->getName() << " failed send to "
                 << Utils::INETAddrToString(destination->address_) << " - buffer size: " << current_->size()
		 << " - " << Utils::showErrno() << std::endl;
    }
    else {
	destination->failures_ = 0;
    }
}

void
StatusEmitterBase::Emitter::foundSlot(const ServiceEntryVector& services)
{
    static Logger::ProcLog log("foundSlot", Log());
    LOGINFO << services.size() << std::endl;

    // We resolve all new services, and only after it is resolved do we submit it to the emitter thread to add
    // it to the vector of Destination objects.
    //
    for (size_t index = 0; index < services.size(); ++index) {
	Zeroconf::ServiceEntry::Ref service(services[index]);
	service->connectToResolvedSignal([this](auto& v){resolvedSlot(v);});
	service->resolve();
    }
}

void
StatusEmitterBase::Emitter::resolvedSlot(const Zeroconf::ServiceEntry::Ref& service)
{
    static Logger::ProcLog log("resolvedSlot", Log());

    const Zeroconf::ResolvedEntry& resolvedEntry = service->getResolvedEntry();
    LOGERROR << "fullname: " << resolvedEntry.getFullName() << " host: " << resolvedEntry.getHost() << " port: "
	     << resolvedEntry.getPort() << " interface: " << service->getInterface() << std::endl;

    std::string host(resolvedEntry.getHost());
    LOGDEBUG << "host: " << host << std::endl;
    
    ACE_INET_Addr address;
    if (address.set(resolvedEntry.getPort(), host.c_str(), 1, AF_INET) == -1) {
	LOGFATAL << "invalid address to use: " << host << '/' << resolvedEntry.getPort() << " - "
                 << Utils::showErrno() << std::endl;
	return;
    }

    // Create a new Destination object to represent the location of the remote StatusCollector. Encapsulate it
    // in an ACE_Message_Block and post it to the emitter thread.
    //
    Destination* dest = new Destination(service);
    dest->address_ = address;
    ACE_Message_Block* data = new ACE_Message_Block(sizeof(*dest), ACE_Message_Block::MB_START, 0,
                                                    reinterpret_cast<char*>(dest));
    if (putq(data) == -1) {
	LOGERROR << "failed to enqueue new destination" << std::endl;
	data->release();
    }
    else {
	++destinationCount_;
	if (timerId_ == -1) {
	    startTimer();
        }
    }
}

size_t
StatusEmitterBase::Emitter::findDestinationIndex(const Zeroconf::ServiceEntry::Ref& service)
{
    // Locate the an existing Destination object that refers to the same StatusCollector object as that
    // represented by the argument.
    //
    for (size_t index = 0; index < destinations_.size(); ++index) {
	Destination* destination = destinations_[index];
	Zeroconf::ServiceEntry::Ref held(destination->serviceEntry_);
	if (held->getName() == service->getName() &&
            held->getDomain() == service->getDomain())
	    return index;
    }

    // Not found. Return the size of the Destination vector as a failure flag.
    //
    return destinations_.size();
}

void
StatusEmitterBase::Emitter::lostSlot(const ServiceEntryVector& services)
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
	ACE_Message_Block* data = new ACE_Message_Block(sizeof(dest), ACE_Message_Block::MB_STOP, 0,
                                                        reinterpret_cast<char*>(dest));
	if (putq(data) == -1) {
	    LOGERROR << "failed to enqueue remove destination" << std::endl;
	    data->release();
	}

	if (destinationCount_) {
	    --destinationCount_;
	    if (! destinationCount_)
		stopTimer();
	}
    }
}

void
StatusEmitterBase::Emitter::startTimer()
{
    Logger::ProcLog log("startTimer", Log());
    LOGDEBUG << "starting status timer" << std::endl;

    // Postpone the first update by a random value between 0 and 1 second.
    //
    ACE_Time_Value delay;
    delay.set(::drand48());

    // Repeat status updates at a fixed update rate.
    //
    ACE_Time_Value repeat;
    repeat.set(owner_.getUpdateRate());

    // Create a timer to periodically post new status to the emitter thread.
    //
    timerId_ = reactor()->schedule_timer(this, 0, delay, repeat);
    LOGERROR << "timerId: " << timerId_ << std::endl;
    if (timerId_ == -1) {
	LOGFATAL << "failed to schdule timer - " << Utils::showErrno() << std::endl;
    }
}

void
StatusEmitterBase::Emitter::stopTimer()
{
    if (timerId_ != -1) {
	reactor()->cancel_timer(timerId_);
	timerId_ = -1;
    }
}

StatusEmitterBase::StatusEmitterBase(const char* emitterType, const char* collectorType, double updateRate)
    : emitterType_(emitterType), collectorType_(collectorType),
      updateRate_(updateRate), emitter_()
{
    ;
}

StatusEmitterBase::~StatusEmitterBase()
{
    close();
}

bool
StatusEmitterBase::open(long threadFlags, long priority)
{
    emitter_.reset(new Emitter(*this));
    return emitter_->openAndInit(threadFlags, priority);
}

void
StatusEmitterBase::close()
{
    if (emitter_) {
	emitter_->close(1);
	emitter_.reset();
    }
}

void
StatusEmitterBase::setUpdateRate(double updateRate)
{
    updateRate_ = updateRate;
    if (emitter_->isEmitting()) {
	emitter_->stopTimer();
	emitter_->startTimer();
    }
}

void
StatusEmitterBase::emitStatus()
{
    if (emitter_) emitter_->emitStatus();
}
