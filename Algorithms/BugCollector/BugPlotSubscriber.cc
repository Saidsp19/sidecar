#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "boost/bind.hpp"

#include "IO/MessageManager.h"
#include "IO/ZeroconfRegistry.h"
#include "Logger/Log.h"
#include "Messages/BugPlot.h"
#include "Zeroconf/ACEMonitor.h"

#include "BugCollector.h"
#include "BugPlotSubscriber.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::BugCollectorUtils;

static int kHeartBeatInterval = 5;	   // seconds

BugPlotReader::BugPlotReader(BugCollector& bugCollector)
    : ACE_Event_Handler(ACE_Reactor::instance()), bugCollector_(bugCollector), serviceName_(""), reader_(),
      metaTypeInfo_(Messages::BugPlot::GetMetaTypeInfo()), heartBeatAddress_(),
      heartBeatWriter_(ACE_INET_Addr(uint16_t(0)))
{
    ;
}

bool
BugPlotReader::openAndInit(const Zeroconf::ServiceEntry::Ref& service)
{
    Logger::ProcLog log("BugPlotReader::openAndInit", bugCollector_.getLog());
    serviceName_ = service->getName();

    const Zeroconf::ResolvedEntry& resolved = service->getResolvedEntry();

    ACE_INET_Addr address;
    if (address.set(resolved.getPort(), resolved.getHost().c_str(), 1, PF_INET) == -1) {
	LOGERROR << "invalid address: " << resolved.getHost() << '/' << resolved.getPort() << std::endl;
	return false;
    }

    if (! reader_.join(address)) {
	LOGERROR << "failed mcast join using address " << resolved.getHost() << '/' << resolved.getPort()
                 << std::endl;
	return false;
    }

    // Obtain the heart-beat port for us to send heart-beat messages to.
    //
    std::istringstream is(resolved.getTextEntry("HeartBeatPort"));
    uint16_t port;
    is >> port;
    heartBeatAddress_.set(port, resolved.getNativeHost().c_str(), 1, PF_INET);

    if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
	LOGERROR << "failed to register input handler" << std::endl;
	return false;
    }

    sendHeartBeat();

    return true;
}

int
BugPlotReader::handle_input(ACE_HANDLE handle)
{
    static Logger::ProcLog log("BugPlotReader::handle_input", bugCollector_.getLog());

    if (! reader_.fetchInput()) return -1;

    if (reader_.isMessageAvailable()) {
	IO::MessageManager manager(reader_.getMessage(), &metaTypeInfo_);
	bugCollector_.process(manager.getNative<Messages::BugPlot>());
    }

#ifdef FIONREAD

    // See if there is more data available to read from the socket. If so, we return 1 so that ACE_Reactor will
    // call us again before doing another ::select().
    //
    int available = 0;
    if (ACE_OS::ioctl (handle, FIONREAD, &available) == 0 && available) {
	LOGDEBUG << "more to come" << std::endl;
	return 1;
    }
#endif

    return 0;
}

int
BugPlotReader::close(u_long flags)
{
    Logger::ProcLog log("BugPlotReader::close", bugCollector_.getLog());
    reactor()->remove_handler(this, ACE_Event_Handler::READ_MASK);
    sendHeartBeat("BYE");
    heartBeatWriter_.close();
    return 0;
}

void
BugPlotReader::sendHeartBeat(const char* msg) const
{
    ssize_t count = ::strlen(msg) + 1;
    heartBeatWriter_.send(msg, count, heartBeatAddress_);
}

ACE_HANDLE
BugPlotReader::get_handle() const
{
    return reader_.getDevice().get_handle();
}

BugPlotSubscriber::BugPlotSubscriber(BugCollector& bugCollector, const std::string& prefix)
    : ACE_Event_Handler(ACE_Reactor::instance()), bugCollector_(bugCollector), browser_(), readers_(),
      prefix_(prefix), timer_(-1)
{
    Zeroconf::ACEMonitorFactory::Ref monitorFactory(Zeroconf::ACEMonitorFactory::Make());
    browser_ = Zeroconf::Browser::Make(monitorFactory,
                                       IO::ZeroconfRegistry::MakeZeroconfType(
                                           IO::ZeroconfRegistry::GetType(IO::ZeroconfRegistry::kPublisher),
                                           Messages::BugPlot::GetMetaTypeInfo().getName()));
    browser_->connectToFoundSignal(boost::bind(&BugPlotSubscriber::foundNotification, this, _1));
    browser_->connectToLostSignal(boost::bind(&BugPlotSubscriber::lostNotification, this, _1));
    browser_->start();

    const ACE_Time_Value repeat(kHeartBeatInterval);
    timer_ = reactor()->schedule_timer(this, 0, repeat, repeat);
}

BugPlotSubscriber::~BugPlotSubscriber()
{
    reactor()->cancel_timer(timer_);
    browser_->stop();
    for (size_t index = 0; index < readers_.size(); ++index) {
	delete readers_[index];
    }
}

void
BugPlotSubscriber::setPrefix(const std::string& prefix)
{
    if (prefix_ != prefix) {
	prefix_ = prefix;
	browser_->stop();
	for (size_t index = 0; index < readers_.size(); ++index) {
	    delete readers_[index];
        }
	readers_.clear();
	browser_->start();
    }
}

void
BugPlotSubscriber::foundNotification(const ServiceEntryVector& services)
{
    for (size_t index = 0; index < services.size(); ++index) {
	Zeroconf::ServiceEntry::Ref service(services[index]);
	if (service->getName().find(prefix_) == 0) {
	    service->connectToResolvedSignal(boost::bind(&BugPlotSubscriber::resolvedNotification, this, _1));
	    service->resolve();
	}
    }
}

void
BugPlotSubscriber::resolvedNotification(const Zeroconf::ServiceEntry::Ref& service)
{
    BugPlotReader* reader = new BugPlotReader(bugCollector_);
    if (reader->openAndInit(service)) {
	readers_.push_back(reader);
    }
    else {
	delete reader;
    }
}

void
BugPlotSubscriber::lostNotification(const ServiceEntryVector& services)
{
    for (size_t index = 0; index < services.size(); ++index) {
	Zeroconf::ServiceEntry::Ref service(services[index]);
	for (size_t inner = 0; inner < readers_.size(); ++inner) {
	    if (service->getName() == readers_[inner]->getServiceName()) {
		readers_[inner]->close(0);
		delete readers_[inner];
		readers_.erase(readers_.begin() + inner);
		break;
	    }
	}
    }
}

int
BugPlotSubscriber::handle_timeout(const ACE_Time_Value& duration,
                                  const void* arg)
{
    for (size_t index = 0; index < readers_.size(); ++index) {
	readers_[index]->sendHeartBeat();
    }
    return 0;
}
