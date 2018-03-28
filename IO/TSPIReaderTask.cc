#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "IO/Module.h"
#include "Logger/Log.h"
#include "Messages/TSPI.h"
#include "Utils/Format.h" // for Utils::showErrno

#include "MessageManager.h"
#include "TSPIReaderTask.h"

using namespace SideCar::IO;

Logger::Log&
TSPIReaderTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.TSPIReaderTask");
    return log_;
}

TSPIReaderTask::Ref
TSPIReaderTask::Make()
{
    Ref ref(new TSPIReaderTask);
    return ref;
}

TSPIReaderTask::TSPIReaderTask() : Super(), reader_(), timer_(-1)
{
    ;
}

bool
TSPIReaderTask::openAndInit(const std::string& host, uint16_t port, int bufferSize, long threadFlags,
                            long threadPriority)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "host: " << host << " port: " << port << " SO_RCVBUV size: " << bufferSize
            << " threadFlags: " << std::hex << threadFlags << std::dec << " threadPriority: " << threadPriority
            << std::endl;

    if (!getTaskName().size()) {
        std::ostringstream os;
        os << "TSPI " << host << '/' << port << " IN";
        setTaskName(os.str());
    }

#ifdef darwin
    if (!bufferSize) bufferSize = 64 * 1024;
#endif

    std::ostringstream os;
    os << "Host: " << host << " Port: " << port << " Buffer Size: " << bufferSize;
    setConnectionInfo(os.str());

    threadFlags_ = threadFlags;
    threadPriority_ = threadPriority;
    setMetaTypeInfoKeyName("TSPI");

    if (!reactor()) reactor(ACE_Reactor::instance());
    address_.set(port, host.c_str(), 1, AF_INET);

    if (bufferSize > 0) {
        ACE_SOCK& device(reader_.getDevice());
        if (device.set_option(SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) == -1) {
            LOGERROR << "failed SO_RCVBUF setting using size of " << bufferSize << " - " << Utils::showErrno()
                     << std::endl;
        }
    }

    msg_queue()->deactivate();

    return scheduleAttempt();
}

bool
TSPIReaderTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return put_next(data, timeout) != -1;
}

int
TSPIReaderTask::svc()
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
            LOGDEBUG << "fetched message" << std::endl;
            Messages::TSPI::Ref msg(Messages::TSPI::MakeRaw("TSPIReaderTask", data));
            data->release();

            if (!msg) {
                LOGWARNING << "ignoring message" << std::endl;
                continue;
            }

            // Update input statistics
            //
            updateInputStats(0, data->length(), msg->getMessageSequenceNumber());

            // We want to setup the message so that it gets delivered to those tasks connected on our first
            // output channel. We also want to update our processing stats.
            //
            MessageManager mgr(msg);
            sendManaged(mgr, 0);
        }
    }

    return 0;
}

int
TSPIReaderTask::close(u_long flags)
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
TSPIReaderTask::attemptConnection()
{
    static Logger::ProcLog log("attemptConnection", Log());
    LOGINFO << std::endl;

    if (!msg_queue()->deactivated()) {
        LOGERROR << "already running" << std::endl;
        if (timer_ != -1) {
            reactor()->cancel_timer(timer_);
            timer_ = -1;
        }
        return;
    }

    enterRunState();

    if (!reader_.join(address_)) {
        LOGERROR << "failed to join multicast stream" << std::endl;
        setError("Failed to join multicast stream");
        if (timer_ == -1) scheduleAttempt();
        return;
    }

    if (timer_ != -1) {
        reactor()->cancel_timer(timer_);
        timer_ = -1;
    }

    msg_queue()->activate();

    if (activate(threadFlags_, 1, 0, threadPriority_) == -1) {
        LOGERROR << "failed to start reader thread" << std::endl;
        setError("Failed to start reader thread.");
        msg_queue()->deactivate();
    }
}

bool
TSPIReaderTask::scheduleAttempt()
{
    static Logger::ProcLog log("scheduleAttempt", Log());
    LOGINFO << std::endl;
    const ACE_Time_Value delay(0);
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
TSPIReaderTask::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    static Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << std::endl;
    attemptConnection();
    return 0;
}
