#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "Logger/Log.h"
#include "Utils/Format.h" // for Utils::showErrno

#include "ClientSocketReaderTask.h"
#include "TCPConnector.h"

using namespace SideCar::IO;

Logger::Log&
ClientSocketReaderTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ClientSocketReaderTask");
    return log_;
}

ClientSocketReaderTask::Ref
ClientSocketReaderTask::Make()
{
    Ref ref(new ClientSocketReaderTask);
    return ref;
}

bool
ClientSocketReaderTask::openAndInit(const std::string& key, const std::string& hostName, uint16_t port)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << key << ' ' << hostName << ' ' << port << std::endl;

    if (!getTaskName().size()) {
        std::ostringstream os;
        os << "ClientSocketReaderTask(" << key << ',' << hostName << '.' << port << ')';
        setTaskName(os.str());
    }

    setMetaTypeInfoKeyName(key);
    if (!reactor()) { reactor(ACE_Reactor::instance()); }

    connector_ = new TCPConnector(this);
    LOGDEBUG << "connector: " << connector_ << std::endl;

    ACE_INET_Addr remoteAddress;
    remoteAddress.set(port, hostName.c_str(), 1, AF_INET);
    if (!connector_->openAndInit(remoteAddress, reactor())) {
        LOGERROR << "failed Connector::open" << std::endl;
        close(1);
        setError("Failed to initialize connector");
        return false;
    }

    return true;
}

int
ClientSocketReaderTask::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << flags << ' ' << connector_ << std::endl;
    if (connector_) {
        connector_->close();
        delete connector_;
        connector_ = 0;
    }

    Task::close(flags);
    return 0;
}
