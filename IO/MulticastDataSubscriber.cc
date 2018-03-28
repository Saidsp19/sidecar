#include <sys/types.h>
#include <unistd.h>

#include <sstream>

#include "ace/Event_Handler.h"
#include "ace/Guard_T.h"
#include "ace/Reactor.h"

#include "Logger/Log.h"
#include "Utils/Format.h"
#include "Zeroconf/ACEMonitor.h"

#include "MessageManager.h"
#include "MulticastDataSubscriber.h"
#include "Readers.h"

static int kHeartBeatInterval = 2;         // seconds
static int kAttemptConnectionInterval = 1; // seconds

namespace SideCar {
namespace IO {

/** Internal class used by MulticastDataSubscriber to perform reading from a multicast UDP socket in a separate
    thread.
*/
class MulticastDataSubscriber::ReaderThread : public ACE_Task<ACE_MT_SYNCH> {
    using Super = ACE_Task<ACE_MT_SYNCH>;

public:
    static Logger::Log& Log();

    /** Constructor.

        \param owner
    */
    ReaderThread(MulticastDataSubscriber* owner) : Super(), owner_(owner), reader_(), active_(false) {}

    /** Initialize the ReaderThread. Attempt to join a multicast broadcast, and if successful, start a new
        thread to read data from the socket.

        \param remoteAddress multicast address to join

        \param bufferSize value to give to SO_RCVBUF socket setting

        \return true if successful, false otherwise
    */
    bool openAndInit(const ACE_INET_Addr& remoteAddress, int bufferSize, long threadFlags, long threadPriority)
    {
        static Logger::ProcLog log("open", Log());
        LOGINFO << "remoteAddress: " << remoteAddress.get_host_addr() << '/' << remoteAddress.get_port_number()
                << std::endl;

        // Attempt to join to the given multicast address to receive data from a publisher.
        //
        if (!reader_.join(remoteAddress)) {
            std::ostringstream os("");
            os << "Failed to join multicast stream from " << remoteAddress.get_host_addr() << "/"
               << remoteAddress.get_port_number() << " - " << errno << ' ' << strerror(errno);
            LOGERROR << owner_->getTaskName() << ' ' << os.str() << std::endl;
            owner_->setError(os.str(), true);
            return false;
        }

        LOGDEBUG << "joined" << std::endl;

        // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers.
        // !!! Apparently, mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management.
        // !!! However, we still need this on Mac OS X (Darwin)
        //

#ifdef darwin
        if (!bufferSize) bufferSize = 64 * 1024;
#endif
        if (bufferSize > 0) {
            ACE_SOCK& device(reader_.getDevice());
            if (device.set_option(SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) == -1) {
                LOGERROR << "failed SO_RCVBUF setting using size of " << bufferSize << " - " << Utils::showErrno()
                         << std::endl;
            }
        }

        // Activate the message queue before we create a new thread, since the thread uses the queue
        // deactivation state as a signal to quit.
        //
        active_ = true;
        if (activate(threadFlags, 1, 0, threadPriority) == -1) {
            active_ = false;
            LOGERROR << owner_->getTaskName() << " failed to start producer thread" << std::endl;
            owner_->setError("Failed to start producer thread");
            return false;
        }

        return true;
    }

    /** Close the connection to a multicast socket. Also called when the service thread exits the svc() method.

        \param flags if non-zero, called by an external entity to shut down the connection; otherwise, called
        due to service thread exiting

        \return -1 if error
    */
    int close(u_long flags = 0)
    {
        static Logger::ProcLog log("close", Log());
        LOGINFO << "flags: " << flags << std::endl;
        if (flags) {
            // Deactivate the message queue, which will signal the thread to exit.
            //
            if (active_) {
                active_ = false;
                wait();
            }

            reader_.close();
        }

        return Super::close(flags);
    }

private:
    /** Method run in a separate thread due to ACE_Task::activate() being invoked. Read data from a multicast
        UDP socket, and if fetched a valid datagram, place onto internal message queue. The read attempt has a
        timeout (see Readers.h), which allows the thread to periodically check for shutdown notification from
        the main thread (the deactivation of the internal message queue).

        \return 0
    */
    int svc()
    {
        static Logger::ProcLog log("svc", Log());
        LOGINFO << owner_->getTaskName() << std::endl;
        ACE_Time_Value timeout(1, 0);
        reader_.setFetchTimeout(&timeout);

        while (active_) {
            if (!reader_.fetchInput()) {
                LOGERROR << "failed fetchInput" << std::endl;
                continue;
            }

            if (!active_) break;

            if (reader_.isMessageAvailable()) { owner_->acquireExternalMessage(reader_.getMessage()); }
        }

        LOGINFO << owner_->getTaskName() << " exiting" << std::endl;
        return 0;
    }

    MulticastDataSubscriber* owner_;
    MulticastSocketReader reader_;
    volatile bool active_;
};

} // namespace IO
} // namespace SideCar

using namespace SideCar::IO;
using namespace SideCar::Zeroconf;

Logger::Log&
MulticastDataSubscriber::ReaderThread::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.MulticastDataSubscriber.ReaderThread");
    return log_;
}

Logger::Log&
MulticastDataSubscriber::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.MulticastDataSubscriber");
    return log_;
}

MulticastDataSubscriber::Ref
MulticastDataSubscriber::Make()
{
    Ref ref(new MulticastDataSubscriber);
    return ref;
}

MulticastDataSubscriber::MulticastDataSubscriber() :
    Super(), address_(), reader_(0), bufferSize_(0), timer_(-1), heartBeatAddress_(),
    heartBeatWriter_(ACE_INET_Addr(uint16_t(0))), closing_(false)
{
    ;
}

MulticastDataSubscriber::~MulticastDataSubscriber()
{
    heartBeatWriter_.close();
}

bool
MulticastDataSubscriber::openAndInit(const std::string& key, const std::string& serviceName, int bufferSize,
                                     int interface, long threadFlags, long threadPriority)
{
    static Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " serviceName: " << serviceName << " bufferSize: " << bufferSize
            << " interface: " << interface << " threadFlags: " << std::hex << threadFlags << std::dec
            << " threadPriority: " << threadPriority << std::endl;

    closing_ = false;
    threadFlags_ = threadFlags;
    threadPriority_ = threadPriority;
    bufferSize_ = bufferSize;

    if (!reactor()) reactor(ACE_Reactor::instance());
    bool ok = Super::openAndInit(key, serviceName, MakeTwinZeroconfType(key), interface);
    return ok;
}

int
MulticastDataSubscriber::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << getTaskName() << " flags: " << flags << std::endl;

    if (flags) {
        ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
        closing_ = true;
        stopTimer();
        stopReader();
    }

    return Super::close(flags);
}

void
MulticastDataSubscriber::setServiceName(const std::string& serviceName)
{
    // The publisher we are subscribed to has changed its name. Change our task name to match.
    //
    Super::setServiceName(serviceName);
    std::string taskName(serviceName);
    taskName += " SUB";
    setTaskName(taskName);
}

void
MulticastDataSubscriber::resolvedService(const Zeroconf::ServiceEntry::Ref& service)
{
    static Logger::ProcLog log("resolvedService", Log());
    const Zeroconf::ResolvedEntry& resolved = service->getResolvedEntry();
    LOGINFO << getTaskName() << " address: " << resolved.getHost() << '/' << resolved.getPort() << std::endl;

    // Record the address for us to subscribe to
    //
    LOGINFO << "resolved address: " << resolved.getHost() << '/' << resolved.getPort() << std::endl;

    {
        ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
        stopReader();
        stopTimer();
        address_.set_port_number(0);
        if (address_.set(resolved.getPort(), resolved.getHost().c_str(), 1, AF_INET) == -1) {
            LOGERROR << "invalid address: " << resolved.getHost() << '/' << resolved.getPort() << std::endl;
            return;
        }
    }

    // Obtain the heart-beat port for us to send heart-beat messages to.
    //
    std::istringstream is(resolved.getTextEntry("HeartBeatPort"));
    uint16_t port;
    is >> port;
    heartBeatAddress_.set(port, resolved.getNativeHost().c_str(), 1, AF_INET);

    // Attempt to enter the last processing state we were in before we had an error. NOTE: this may invoke our
    // setUsingData() method which may attempt to acquire our mutex. BE CAREFUL!
    //
    establishedConnection();
}

void
MulticastDataSubscriber::lostService()
{
    static Logger::ProcLog log("lostService", Log());
    LOGINFO << getTaskName() << std::endl;

    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

    // Stop the reader, and the timer. It will restart once resolvedService() executes
    //
    stopReader();
    stopTimer();
    address_.set_port_number(0);
}

void
MulticastDataSubscriber::stopReader()
{
    static Logger::ProcLog log("stopReader", Log());
    LOGINFO << getTaskName() << " reader: " << reader_ << std::endl;

    if (reader_) {
        reader_->close(1);
        sendHeartBeat("BYE");
        delete reader_;
        reader_ = 0;
    }
}

void
MulticastDataSubscriber::attemptConnection()
{
    static Logger::ProcLog log("attemptConnection", Log());
    LOGINFO << getTaskName() << std::endl;

    // NOTE: this could be reentered after a successful ReaderThread launch when calling
    // establishedConnection(). BE CAREFUL!
    //
    if (reader_ || address_.get_port_number() == 0 || !getServiceEntry()) return;

    // Attempt to subscribe to the published multicast address and start the reader thread.
    //
    reader_ = new ReaderThread(this);

    if (!reader_->openAndInit(address_, bufferSize_, threadFlags_, threadPriority_)) {
        LOGERROR << getTaskName() << " failed to start reader thread" << std::endl;

        // Clean up anything left over from trying to open the reader.
        //
        stopReader();

        // Make sure the timer is set to the connection attempt interval.
        //
        startTimer(kAttemptConnectionInterval);
        return;
    }

    // Notify our parent class that we are now connected.
    //
    establishedConnection();
    sendHeartBeat("HI");

    // Revise the timer to use the heart-beat interval and send the publisher our first heart-beat message.
    //
    startTimer(kHeartBeatInterval);
}

void
MulticastDataSubscriber::startTimer(int interval)
{
    static Logger::ProcLog log("startTimer", Log());
    LOGINFO << "interval: " << interval << std::endl;

    stopTimer();
    const ACE_Time_Value repeat(interval);
    const ACE_Time_Value delay(interval);
    timer_ = reactor()->schedule_timer(this, &timer_, delay, repeat);
    LOGDEBUG << getTaskName() << " started timer " << timer_ << std::endl;
}

void
MulticastDataSubscriber::stopTimer()
{
    if (timer_ != -1) {
        reactor()->cancel_timer(timer_);
        timer_ = -1;
    }
}

int
MulticastDataSubscriber::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    static Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << getTaskName() << " reader: " << reader_ << " arg: " << arg << " timer: " << timer_ << std::endl;

    if (arg != &timer_) return Super::handle_timeout(duration, arg);

    if (closing_) {
        ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
        stopTimer();
        return 0;
    }

    if (!reader_) {
        ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
        attemptConnection();
    } else {
        sendHeartBeat("HI");
    }

    return 0;
}

void
MulticastDataSubscriber::sendHeartBeat(const char* msg) const
{
    static Logger::ProcLog log("sendHeartBeat", Log());
    LOGINFO << getTaskName() << ' ' << msg << std::endl;
    ssize_t count = ::strlen(msg) + 1;
    if (heartBeatWriter_.send(msg, count, heartBeatAddress_) != count) {
        LOGERROR << "failed to send heart-beat message - " << errno << ": " << ::strerror(errno) << std::endl;
        LOGERROR << "address: " << heartBeatAddress_.get_host_addr() << " port: " << heartBeatAddress_.get_port_number()
                 << std::endl;
    }
}

void
MulticastDataSubscriber::setUsingData(bool state)
{
    static Logger::ProcLog log("setUsingData", Log());
    LOGINFO << getTaskName() << " old: " << isUsingData() << " new: " << state << std::endl;

    // Update the valve state first. We don't muck with isOpen, we just act on its value.
    //
    Super::setUsingData(state);

    if (isUsingData()) {
        // Only attempt to connect if we are not already connected and we have resolved connection information.
        //
        if (!closing_ && !reader_) {
            ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
            attemptConnection();
        }
    } else {
        // Shutdown the reader and heart-beat timer if they are active.
        //
        if (reader_) {
            ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
            stopReader();
            stopTimer();
        }
    }
}
