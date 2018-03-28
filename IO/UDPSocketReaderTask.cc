#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "IO/Module.h"
#include "Logger/Log.h"

#include "MessageManager.h"
#include "UDPSocketReaderTask.h"

using namespace SideCar::IO;

Logger::Log&
UDPSocketReaderTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.UDPSocketReaderTask");
    return log_;
}

UDPSocketReaderTask::Ref
UDPSocketReaderTask::Make(size_t messageSize)
{
    Ref ref(new UDPSocketReaderTask(messageSize));
    return ref;
}

UDPSocketReaderTask::UDPSocketReaderTask(size_t messageSize) : IOTask(), reader_(messageSize)
{
    ;
}

bool
UDPSocketReaderTask::openAndInit(const std::string& key, uint16_t port)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << key << ' ' << port << std::endl;

    std::ostringstream os;
    os << "Port: " << port;
    setConnectionInfo(os.str());

    if (!getTaskName().size()) {
        std::ostringstream os;
        os << "UDPIn(" << key << ',' << port << ')' << std::endl;
        setTaskName(os.str());
    }

    setMetaTypeInfoKeyName(key);

    if (!reactor()) reactor(ACE_Reactor::instance());
    ACE_INET_Addr address(port);
    if (!reader_.open(address)) {
        LOGERROR << "failed open" << std::endl;
        return false;
    }

    LOGDEBUG << "handle: " << reader_.getDevice().get_handle() << std::endl;
    if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
        LOGERROR << "failed to register input handler" << std::endl;
        return false;
    }

    return true;
}

bool
UDPSocketReaderTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return put_next(data, timeout) != -1;
}

int
UDPSocketReaderTask::handle_input(ACE_HANDLE handle)
{
    static Logger::ProcLog log("handle_input", Log());
    LOGDEBUG << std::endl;

    if (!reader_.fetchInput()) {
        LOGERROR << "EOF on file" << std::endl;
        return -1;
    }

    if (reader_.isMessageAvailable()) { acquireExternalMessage(reader_.getMessage()); }

    return 0;
}

int
UDPSocketReaderTask::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;
    int rc = reactor()->remove_handler(this, ACE_Event_Handler::READ_MASK);
    LOGDEBUG << "remove_handler: " << rc << std::endl;
    return 0;
}
