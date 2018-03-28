#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "IO/Module.h"
#include "Logger/Log.h"

#include "MessageManager.h"
#include "ServerSocketReaderTask.h"

using namespace SideCar::IO;

Logger::Log&
ServerSocketReaderTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ServerSocketReaderTask");
    return log_;
}

ServerSocketReaderTask::Ref
ServerSocketReaderTask::Make()
{
    Ref ref(new ServerSocketReaderTask);
    return ref;
}

int
ServerSocketReaderTask::InputHandler::open(void* arg)
{
    Logger::ProcLog log("InputHandler::open", Log());
    LOGDEBUG << arg << std::endl;

    Acceptor* acceptor = static_cast<Acceptor*>(arg);
    task_ = acceptor->getTask();

    if (Super::open(arg) == -1) {
        LOGERROR << "failed to open input handler" << std::endl;
        return -1;
    }

    LOGDEBUG << "handle: " << get_handle() << std::endl;

    // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers. Apparently,
    // !!! mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management. However, we still need
    // !!! this on Mac OS X (Darwin)
    //
#ifdef darwin
    int bufferSize = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
    peer().set_option(SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
#endif

    // Setup the socket reader to use our socket connection
    //
    reader_ = new TCPSocketReader;
    reader_->getDevice().set_handle(get_handle());

    return 0;
}

int
ServerSocketReaderTask::InputHandler::handle_input(ACE_HANDLE handle)
{
    Logger::ProcLog log("InputHandler::handle_input", Log());
    LOGINFO << handle << std::endl;

    if (!reader_->fetchInput()) {
        LOGDEBUG << "failed to read data from socket" << std::endl;
        return -1;
    }

    if (!reader_->isMessageAvailable()) return 0;

    task_->acquireExternalMessage(reader_->getMessage());

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
ServerSocketReaderTask::InputHandler::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << "flags: " << flags << std::endl;

    if (reader_) {
        delete reader_;
        reader_ = 0;
    }

    return Super::close(flags);
}

ServerSocketReaderTask::ServerSocketReaderTask() : IOTask(), acceptor_(0), port_(0)
{
    Logger::ProcLog log("ServerSocketReaderTask", Log());
    LOGINFO << std::endl;
}

bool
ServerSocketReaderTask::openAndInit(const std::string& key, uint16_t port)
{
    Logger::ProcLog log("open", Log());
    LOGINFO << " key: " << key << " port: " << port << std::endl;

    if (!getTaskName().size()) {
        std::ostringstream os;
        os << "ServerSocketReaderTask(" << key << ',' << port;
        setTaskName(os.str());
    }

    setMetaTypeInfoKeyName(key);
    if (!reactor()) reactor(ACE_Reactor::instance());

    acceptor_ = new Acceptor(this);

    ACE_INET_Addr serverAddress(port);
    if (acceptor_->open(serverAddress) == -1) {
        LOGERROR << "failed to open server socket on port " << port << std::endl;
        delete acceptor_;
        acceptor_ = 0;
        return false;
    }

    acceptor_->acceptor().get_local_addr(serverAddress);
    port_ = serverAddress.get_port_number();
    LOGDEBUG << "port: " << port_ << std::endl;

    LOGDEBUG << "done" << std::endl;
    return true;
}

bool
ServerSocketReaderTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return put_next(data, timeout) != -1;
}

int
ServerSocketReaderTask::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;
    if (flags && acceptor_) {
        acceptor_->close();
        delete acceptor_;
        acceptor_ = 0;
    }
    return Super::close(flags);
}
