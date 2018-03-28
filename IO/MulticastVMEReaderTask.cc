#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"
#include "ace/Sched_Params.h"

#include "IO/Module.h"
#include "Logger/Log.h"
#include "Messages/RawVideo.h"
#include "Utils/Format.h" // for Utils::showErrno

#include "MessageManager.h"
#include "MulticastVMEReaderTask.h"

using namespace SideCar::IO;

Logger::Log&
MulticastVMEReaderTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.MulticastVMEReaderTask");
    return log_;
}

MulticastVMEReaderTask::Ref
MulticastVMEReaderTask::Make()
{
    Ref ref(new MulticastVMEReaderTask);
    return ref;
}

MulticastVMEReaderTask::MulticastVMEReaderTask() : Super(), reader_(), bufferSize_(0), timer_(-1)
{
    msg_queue()->deactivate();
}

bool
MulticastVMEReaderTask::openAndInit(const std::string& host, uint16_t port, int bufferSize, long threadFlags,
                                    long threadPriority)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "host: " << host << " port: " << port << " SO_RCVBUV size: " << bufferSize
            << " threadFlags: " << std::hex << threadFlags << std::dec << " threadPriority: " << threadPriority
            << std::endl;

    if (!getTaskName().size()) {
        std::ostringstream os;
        os << "VME " << host << '/' << port << " SUB";
        setTaskName(os.str());
    }

#ifdef darwin
    if (!bufferSize) bufferSize = 64 * 1024; // 64K IO buffer
#endif

    bufferSize_ = bufferSize;
    threadFlags_ = threadFlags;
    threadPriority_ = threadPriority;
    setMetaTypeInfoKeyName("RawVideo");

    if (!reactor()) reactor(ACE_Reactor::instance());

    address_ = ACE_INET_Addr(port, host.c_str());

    msg_queue()->deactivate();

    return scheduleAttempt();
}

bool
MulticastVMEReaderTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return put_next(data, timeout) != -1;
}

int
MulticastVMEReaderTask::svc()
{
    static Logger::ProcLog log("svc", Log());
    LOGINFO << std::endl;

    // Keep running until to stop, or unable to read from the socket.
    //
    ACE_Time_Value timeout(1, 0);
    reader_.setFetchTimeout(&timeout);

    while (!msg_queue()->deactivated()) {
        reader_.fetchInput();
        if (reader_.isMessageAvailable()) {
            ACE_Message_Block* data = reader_.getMessage();
            LOGDEBUG << getTaskName() << " getMessage: " << data << std::endl;

            // Create a RawVideo message using raw VME data
            //
            Messages::RawVideo::Ref msg(Messages::RawVideo::Make("MulticastVMEReader", data));

            // Update input statistics for this task.
            //
            updateInputStats(0, data->length(), msg->getMessageSequenceNumber());
            MessageManager mgr(msg);
            sendManaged(mgr, 0);
        }
    }

    LOGERROR << getTaskName() << " exiting" << std::endl;
    return 0;
}

int
MulticastVMEReaderTask::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;

    // Is this the thread closing?
    //
    if (flags) {
        if (timer_ != -1) {
            reactor()->cancel_timer(timer_);
            timer_ = -1;
        }

        if (!msg_queue()->deactivated()) {
            msg_queue()->deactivate();
            wait();
        }

        reader_.close();
    }

    return Super::close(flags);
}

void
MulticastVMEReaderTask::attemptConnection()
{
    static Logger::ProcLog log("attemptConnection", Log());
    LOGINFO << getTaskName() << std::endl;

    if (!msg_queue()->deactivated()) {
        LOGERROR << getTaskName() << "already running" << std::endl;
        if (timer_ != -1) {
            reactor()->cancel_timer(timer_);
            timer_ = -1;
        }
        return;
    }

    enterRunState();

    if (!reader_.join(address_)) {
        LOGERROR << getTaskName() << " failed to join multicast stream" << std::endl;
        setError("Failed to join multicast stream");
        if (timer_ == -1) scheduleAttempt();
        return;
    }

    if (timer_ != -1) {
        reactor()->cancel_timer(timer_);
        timer_ = -1;
    }

    if (bufferSize_ > 0) {
        ACE_SOCK& device(reader_.getDevice());
        int rc = device.set_option(SOL_SOCKET, SO_RCVBUF, &bufferSize_, sizeof(bufferSize_));
        if (rc == -1)
            LOGERROR << getTaskName() << "failed SO_RCVBUF setting using size of " << bufferSize_ << " - "
                     << Utils::showErrno() << std::endl;
    }

    msg_queue()->activate();

    LOGDEBUG << getTaskName() << "threadFlags: " << std::hex << threadFlags_ << std::dec << std::endl;

    if (activate(threadFlags_, 1, 0, threadPriority_) == -1) {
        LOGERROR << "failed to start reader thread" << std::endl;
        setError("Failed to start reader thread.");
        msg_queue()->deactivate();
    }
}

bool
MulticastVMEReaderTask::scheduleAttempt()
{
    static Logger::ProcLog log("scheduleAttempt", Log());
    LOGINFO << getTaskName() << ' ' << timer_ << std::endl;
    const ACE_Time_Value delay(5);
    const ACE_Time_Value repeat(5);
    timer_ = reactor()->schedule_timer(this, 0, delay, repeat);
    if (timer_ == -1) {
        LOGERROR << "failed to schedule timer for reconnect" << std::endl;
        setError("Failed to schedule reconnection");
        return false;
    }
    return true;
}

int
MulticastVMEReaderTask::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    static Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << getTaskName() << ' ' << timer_ << std::endl;
    attemptConnection();
    return 0;
}
