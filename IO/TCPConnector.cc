#include "Logger/Log.h"

#include "TCPConnector.h"

using namespace SideCar::IO;

Logger::Log&
TCPConnector::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.TCPConnector");
    return log_;
}

bool
TCPConnector::openAndInit(const ACE_INET_Addr& remoteAddress, ACE_Reactor* reactor)
{
    Logger::ProcLog log("open", Log());
    LOGINFO << std::endl;

    remoteAddress_ = remoteAddress;

    if (Super::open(reactor) == -1) {
        LOGERROR << "failed to initialize" << std::endl;
        close();
        return false;
    }

    if (!attemptConnection()) {
        LOGERROR << "failed to start connection attempt" << std::endl;
        close();
        return false;
    }

    LOGDEBUG << "EXIT" << std::endl;
    return true;
}

int
TCPConnector::close()
{
    Logger::ProcLog log("close", Log());
    LOGINFO << std::endl;
    inputHandler_.close(1);
    reactor()->cancel_timer(this);
    return 0;
}

bool
TCPConnector::attemptConnection()
{
    static Logger::ProcLog log("attemptConnection", Log());
    LOGINFO << std::endl;

    // Reuse the same InputHandler object for all (re)connections.
    //
    TCPInputHandler* inputHandler = &inputHandler_;
    int rc = Super::connect(inputHandler, remoteAddress_);
    if (rc == -1) {
        std::ostringstream os;
        os << "Failed to connect to " << remoteAddress_.get_host_name() << "/" << remoteAddress_.get_port_number();
        LOGERROR << os.str() << std::endl;
        task_->setError(os.str());

        if (timer_ == -1)
            if (!scheduleConnectionAttempt()) return false;
    } else {
        LOGDEBUG << "established connection" << std::endl;

        if (timer_ != -1) {
            reactor()->cancel_timer(timer_);
            timer_ = -1;
        }

        task_->establishedConnection();
        task_->setUsingData(task_->isUsingData());
    }

    LOGDEBUG << "EXIT" << std::endl;
    return true;
}

bool
TCPConnector::scheduleConnectionAttempt()
{
    static Logger::ProcLog log("scheduleConnectionAttempt", Log());
    LOGINFO << std::endl;
    const ACE_Time_Value delay(2);
    const ACE_Time_Value repeat(2);
    timer_ = reactor()->schedule_timer(this, 0, delay, repeat);
    if (timer_ == -1) {
        LOGERROR << "failed to schedule timer for reconnect attempt" << std::endl;
        return false;
    }

    return true;
}

int
TCPConnector::handle_timeout(const ACE_Time_Value& duration, const void* arg)
{
    static Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << std::endl;
    return attemptConnection() ? 0 : -1;
}
