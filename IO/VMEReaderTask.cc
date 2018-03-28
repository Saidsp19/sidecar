#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "IO/Module.h"
#include "Logger/Log.h"
#include "Messages/RawVideo.h"

#include "MessageManager.h"
#include "VMEReaderTask.h"

using namespace SideCar::IO;

Logger::Log&
VMEReaderTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.VMEReaderTask");
    return log_;
}

VMEReaderTask::Ref
VMEReaderTask::Make()
{
    Ref ref(new VMEReaderTask);
    return ref;
}

VMEReaderTask::VMEReaderTask() : IOTask(), reader_()
{
    ;
}

bool
VMEReaderTask::openAndInit(const std::string& host, uint16_t port, int bufferSize)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << host << ' ' << port << std::endl;

    if (!getTaskName().size()) {
        std::ostringstream os;
        os << "VME " << host << '/' << port << " SUB";
        setTaskName(os.str());
    }

    setMetaTypeInfoKeyName("RawVideo");

    if (!reactor()) reactor(ACE_Reactor::instance());
    ACE_INET_Addr address(port, host.c_str());

    if (!reader_.join(address)) {
        LOGERROR << "failed open" << std::endl;
        return false;
    }

    LOGDEBUG << "handle: " << reader_.getDevice().get_handle() << std::endl;
    if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
        LOGERROR << "failed to register input handler" << std::endl;
        return false;
    }

    // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers. Apparently,
    // !!! mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management. However, we still need
    // !!! this on Mac OS X (Darwin)
    //
#ifdef darwin
    if (!bufferSize) bufferSize = 64 * 1024; // 64K buffer
#endif

    if (bufferSize) {
        ACE_SOCK& sock(reader_.getDevice());
        int rc = sock.set_option(SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
        if (rc == -1) LOGERROR << "failed SO_RCVBUF setting using size of " << bufferSize << std::endl;
    }

    return true;
}

bool
VMEReaderTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return put_next(data, timeout) != -1;
}

int
VMEReaderTask::handle_input(ACE_HANDLE handle)
{
    Logger::ProcLog log("handle_input", Log());
    LOGDEBUG << std::endl;

    if (!reader_.fetchInput()) {
        LOGERROR << "EOF on file" << std::endl;
        return -1;
    }

    if (reader_.isMessageAvailable()) {
        ACE_Message_Block* data = reader_.getMessage();
        Messages::RawVideo::Ref msg(Messages::RawVideo::Make("VMEReaderTask", data));
        MessageManager mgr(msg);
        acquireExternalMessage(mgr.getMessage());
    }

#ifdef FIONREAD

    // See if there is more data available to read from the socket. If so, we return 1 so that ACE_Reactor will
    // call us again before doing another ::select().
    //
    int available = 0;
    if (ACE_OS::ioctl(handle, FIONREAD, &available) == 0 && available) {
        LOGDEBUG << "more to come" << std::endl;
        return 1;
    }

#endif

    return 0;
}

int
VMEReaderTask::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;
    int rc = reactor()->remove_handler(this, ACE_Event_Handler::READ_MASK);
    LOGDEBUG << "remove_handler: " << rc << std::endl;
    return 0;
}
