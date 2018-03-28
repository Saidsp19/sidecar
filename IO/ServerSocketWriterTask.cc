#include <sstream>

#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include "Logger/Log.h"

#include "GatherWriter.h"
#include "MessageManager.h"
#include "Module.h"
#include "ServerSocketWriterTask.h"

using namespace SideCar::IO;

Logger::Log&
ServerSocketWriterTask::OutputHandler::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ServerSocketWriterTask.OutputHandler");
    return log_;
}

ServerSocketWriterTask::OutputHandler::~OutputHandler()
{
    Logger::ProcLog log("~OutputHandler", Log());
    LOGDEBUG << std::endl;
}

int
ServerSocketWriterTask::OutputHandler::open(void* arg)
{
    Logger::ProcLog log("open", Log());
    LOGDEBUG << arg << ' ' << get_handle() << std::endl;

    Acceptor* acceptor = static_cast<Acceptor*>(arg);
    task_ = acceptor->getTask();

    // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers. Apparently,
    // !!! mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management. However, we still need
    // !!! this on Mac OS X (Darwin)
    //
    int bufferSize = task_->getBufferSize();
#ifdef darwin
    if (!bufferSize) bufferSize = 64 * 1024;
#endif

    if (bufferSize) {
        if (peer().set_option(SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) == -1)
            LOGERROR << "failed to set SO_SNDBUF to " << bufferSize << std::endl;
    }

    // Register for notification when the socket in closed.
    //
    if (reactor()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
        LOGERROR << "failed to register output handler" << std::endl;
        return -1;
    }

    // Change our writer to use our socket connection
    //
    writer_.getDevice().set_handle(get_handle());

    task_->addOutputHandler(this);

    return activate(task_->threadFlags_, 1, 0, task_->threadPriority_);
}

int
ServerSocketWriterTask::OutputHandler::handle_input(ACE_HANDLE handle)
{
    Logger::ProcLog log("handle_input", Log());
    LOGINFO << "remote connection closed" << std::endl;

    // Disable the input queue so that the svc thread will exit.
    //
    msg_queue()->deactivate();
    return 0;
}

int
ServerSocketWriterTask::OutputHandler::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << "closing - " << flags << std::endl;

    if (flags) {
        if (!msg_queue()->deactivated()) {
            LOGDEBUG << "shutting down service thread" << std::endl;
            msg_queue()->deactivate();
            if (wait() == -1) LOGERROR << "failed to join to service thread" << std::endl;
        }
    } else {
        LOGDEBUG << "closing client connection" << std::endl;

        // The thread has finished. Close the client connection, unregister ourselves for the connection device.
        //
        reactor()->remove_handler(get_handle(), ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL);
        peer().close();

        // Place a message on our master's message queue that will remove us from its set of clients.
        //
        task_->removeOutputHandler(this);
    }

    LOGINFO << "done" << std::endl;
    return 0;
}

int
ServerSocketWriterTask::OutputHandler::svc()
{
    Logger::ProcLog log("svc", Log());
    LOGINFO << "start" << std::endl;

    GatherWriter gatherWriter(writer_);
    gatherWriter.setCountLimit(1);

    ACE_Message_Block* data = 0;
    while (getq(data) != -1) {
        MessageManager mgr(data);
        if (!mgr.hasNativeMessageType(task_->getMetaTypeInfoKey())) {
            LOGFATAL << "invalid message type in queue - " << mgr.getMessageType() << std::endl;
            ::abort();
        }

        gatherWriter.add(mgr.getEncoded());
    }

    gatherWriter.flush();

    LOGWARNING << "terminating" << std::endl;
    return 0;
}

Logger::Log&
ServerSocketWriterTask::Acceptor::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ServerSocketWriterTask.Acceptor");
    return log_;
}

ServerSocketWriterTask::Acceptor::~Acceptor()
{
    Logger::ProcLog log("~Acceptor", Log());
    LOGDEBUG << std::endl;
}

int
ServerSocketWriterTask::Acceptor::handle_input(ACE_HANDLE listener)
{
    Logger::ProcLog log("handle_input", Log());
    LOGINFO << std::endl;
    return Super::handle_input(listener);
}

int
ServerSocketWriterTask::Acceptor::make_svc_handler(OutputHandler*& sh)
{
    Logger::ProcLog log("make_svc_handler", Log());
    LOGINFO << "sh: " << sh << std::endl;
    return Super::make_svc_handler(sh);
}

Logger::Log&
ServerSocketWriterTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ServerSocketWriterTask");
    return log_;
}

ServerSocketWriterTask::Ref
ServerSocketWriterTask::Make()
{
    Ref ref(new ServerSocketWriterTask);
    return ref;
}

ServerSocketWriterTask::ServerSocketWriterTask() : IOTask(), acceptor_(0), clients_()
{
    Logger::ProcLog log("ServerSocketWriterTask", Log());
    LOGINFO << std::endl;
}

ServerSocketWriterTask::~ServerSocketWriterTask()
{
    Logger::ProcLog log("~ServerSocketWriterTask", Log());
    LOGINFO << std::endl;
}

bool
ServerSocketWriterTask::openAndInit(const std::string& key, uint16_t port, int bufferSize, long threadFlags,
                                    long threadPriority)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " port: " << port << " threadFlags: " << std::hex << threadFlags << std::dec
            << " threadPriority: " << threadPriority << std::endl;

    bufferSize_ = bufferSize;
    threadFlags_ = threadFlags;
    threadPriority_ = threadPriority;

    if (!getTaskName().size()) {
        std::ostringstream os;
        os << "ServerSocketWriterTask(" << key << ',' << port << ')';
        setTaskName(os.str());
    }

    setMetaTypeInfoKeyName(key);

    if (!reactor()) reactor(ACE_Reactor::instance());
    msg_queue()->activate();

    acceptor_ = new Acceptor(this);

    ACE_INET_Addr serverAddress(port);
    if (acceptor_->open(serverAddress) == -1) {
        LOGERROR << "failed to open server on port " << port << std::endl;
        delete acceptor_;
        acceptor_ = 0;
        return false;
    }

    // !!! NOTE: recent Linux kernels (2.6.?) do a very good job of managing network buffers. Apparently,
    // !!! mucking with SO_RCVBUF and SO_SNDBUF will disable the automatic management. However, we still need
    // !!! this on Mac OS X (Darwin)
    //
#ifdef darwin
    if (bufferSize == 0) bufferSize = 64 * 1024; // 64K
#endif

    if (bufferSize) {
        if (acceptor_->acceptor().set_option(SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) == -1)
            LOGERROR << "failed to set SO_SNDBUF to " << bufferSize << std::endl;
    }

    acceptor_->acceptor().get_local_addr(serverAddress);
    port_ = serverAddress.get_port_number();
    LOGDEBUG << "port: " << port_ << std::endl;

    if (activate(threadFlags_, 1, 0, threadPriority_) == -1) {
        LOGERROR << "failed to start main processing thread" << std::endl;
        return false;
    }

    LOGDEBUG << "done" << std::endl;
    return true;
}

int
ServerSocketWriterTask::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << "shutting down - " << flags << std::endl;

    if (flags && acceptor_) {
        LOGDEBUG << "closing acceptor" << std::endl;
        acceptor_->close();
        delete acceptor_;
        acceptor_ = 0;

        LOGDEBUG << "shutting down svc thread" << std::endl;
        msg_queue()->deactivate();
        if (wait() == -1) LOGERROR << "failed to join main processing thread" << std::endl;

        // Now that our main processing thread is finished, we can safely muck with our client output threads.
        // First, handle the any unprocessed client open/close requests.
        //
        msg_queue()->activate();
        while (!msg_queue()->is_empty()) {
            ACE_Message_Block* data;
            if (getq(data) != -1) updateClients(data);
        }

        // Now, close any remaining client output threads
        //
        for (size_t index = 0; index < clients_.size(); ++index) {
            OutputHandler* handler = clients_[index];
            LOGDEBUG << "closing output handler: " << handler << std::endl;
            handler->close(1);
            delete handler;
        }

        clients_.clear();
        connectionCountChangedSignal_(0);
    }

    return Task::close(flags);
}

void
ServerSocketWriterTask::addOutputHandler(OutputHandler* handler)
{
    Logger::ProcLog log("addOutputHandler", Log());
    LOGINFO << handler << std::endl;
    ACE_Message_Block* req = new ACE_Message_Block(sizeof(handler), ACE_Message_Block::MB_START, 0, (char*)handler);
    req->wr_ptr(sizeof(handler));
    if (putq(req) == -1) req->release();
}

void
ServerSocketWriterTask::removeOutputHandler(OutputHandler* handler)
{
    Logger::ProcLog log("removeOutputHandler", Log());
    LOGINFO << handler << std::endl;
    ACE_Message_Block* req = new ACE_Message_Block(sizeof(handler), ACE_Message_Block::MB_HANGUP, 0, (char*)handler);
    req->wr_ptr(sizeof(handler));
    if (putq(req) == -1) req->release();
}

bool
ServerSocketWriterTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    Logger::ProcLog log("deliverDataMessage", Log());
    LOGINFO << data << std::endl;

    if (clients_.empty()) {
        data->release();
        return true;
    }

    return putq(data, timeout) != -1;
}

int
ServerSocketWriterTask::svc()
{
    static Logger::ProcLog log("svc", Log());
    LOGINFO << "started" << std::endl;

    ACE_Message_Block* data;
    while (getq(data) != -1) {
        // For normal data messages, just distribute copies to each of the active output threads.
        //
        if (MessageManager::IsDataMessage(data)) {
            distribute(data);
        } else {
            updateClients(data);
        }
    }

    LOGWARNING << "exiting" << std::endl;

    return 0;
}

void
ServerSocketWriterTask::updateClients(ACE_Message_Block* data)
{
    Logger::ProcLog log("updateClients", Log());

    size_t oldSize = clients_.size();
    LOGINFO << "client size: " << oldSize << std::endl;

    OutputHandler* outputHandler = reinterpret_cast<OutputHandler*>(data->rd_ptr());

    if (data->msg_type() == ACE_Message_Block::MB_START) {
        LOGDEBUG << "added new output handler" << std::endl;
        clients_.push_back(outputHandler);
    } else if (data->msg_type() == ACE_Message_Block::MB_HANGUP) {
        OutputHandlerVector::iterator pos = std::find(clients_.begin(), clients_.end(), outputHandler);
        if (pos == clients_.end()) {
            LOGERROR << "did not find client to remove" << std::endl;
        } else {
            (*pos)->close(1);
            delete *pos;
            clients_.erase(pos);
        }

        LOGDEBUG << "removed output handler" << std::endl;
    } else {
        LOGERROR << "invalid message type - " << data->msg_type() << std::endl;
    }

    data->release();

    if (oldSize != clients_.size()) {
        LOGDEBUG << "new client size: " << clients_.size() << std::endl;
        connectionCountChangedSignal_(clients_.size());
    }
}

void
ServerSocketWriterTask::distribute(ACE_Message_Block* data)
{
    static Logger::ProcLog log("distribute", Log());

    MessageManager mgr(data);

    OutputHandlerVector::const_iterator pos = clients_.begin();
    OutputHandlerVector::const_iterator end = clients_.end();
    for (; pos != end; ++pos) {
        LOGDEBUG << "adding to client output queue" << std::endl;
        ACE_Message_Block* dup = mgr.getMessage();
        if ((*pos)->put(dup) == -1) {
            LOGERROR << "failed to write to client output queue - flushing" << std::endl;
            dup->release();
            (*pos)->close(1);
        }
    }
}
