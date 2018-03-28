#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "Logger/Log.h"

#include "FileWriterTask.h"
#include "GatherWriter.h"
#include "MessageManager.h"
#include "Module.h"

using namespace SideCar::IO;

Logger::Log&
FileWriterTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.FileWriterTask");
    return log_;
}

FileWriterTask::Ref
FileWriterTask::Make()
{
    Ref ref(new FileWriterTask);
    return ref;
}

bool
FileWriterTask::openAndInit(const std::string& key, const std::string& path, bool acquireBasisTimeStamps,
                            long threadFlags, long threadPriority)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " path: " << path << " acquireBasisTimeStamps: " << acquireBasisTimeStamps
            << " threadFlags: " << std::hex << threadFlags << std::dec << " threadPriority: " << threadPriority
            << std::endl;

    acquireBasisTimeStamps_ = acquireBasisTimeStamps;

    if (!getTaskName().size()) {
        std::string taskName("FileWriterTask(");
        taskName += key;
        taskName += ',';
        taskName += path;
        taskName += ')';
        setTaskName(taskName);
    }

    setMetaTypeInfoKeyName(key);
    if (!reactor()) reactor(ACE_Reactor::instance());

    ACE_FILE_Addr filePath(path.c_str());
    ACE_FILE_Connector connector;
    if (connector.connect(writer_.getDevice(), filePath, 0, ACE_Addr::sap_any, 0, O_WRONLY | O_CREAT,
                          ACE_DEFAULT_FILE_PERMS) == -1) {
        LOGERROR << "failed to open file " << path << std::endl;
        return false;
    }

    if (activate(threadFlags, 1, 0, threadPriority) == -1) {
        LOGERROR << "failed to activate new thread - " << errno << std::endl;
        return false;
    }

    establishedConnection();

    return true;
}

bool
FileWriterTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return putq(data, timeout) != -1;
}

int
FileWriterTask::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;

    Task::close(flags);

    if (flags) {
        msg_queue()->deactivate();
        wait();
        writer_.close();
    }

    // Service thread has exited.
    //
    return 0;
}

int
FileWriterTask::svc()
{
    Logger::ProcLog log("svc", Log());
    LOGINFO << "starting" << std::endl;

    GatherWriter gatherWriter(writer_);
    gatherWriter.setSizeLimit(32 * 1024);

    ACE_Message_Block* data = 0;
    while (gatherWriter.isOK() && getq(data) != -1) {
        MessageManager mgr(data);
        if (!mgr.hasNativeMessageType(getMetaTypeInfoKey())) {
            LOGFATAL << "invalid message type in queue - " << mgr.getMessageType() << std::endl;
            ::abort();
        }

        if (acquireBasisTimeStamps_) {
            Messages::Header::Ref msg(mgr.getNative());
            Messages::Header::Ref basis(msg->getBasis());
            if (basis) {
                while (basis->getBasis()) { basis = basis->getBasis(); }
                msg->setCreatedTimeStamp(basis->getCreatedTimeStamp());
            } else {
                LOGWARNING << "no message basis" << std::endl;
            }
        }

        if (!gatherWriter.add(mgr.getEncoded())) { break; }
    }

    LOGWARNING << "terminating" << std::endl;

    // Flush anything remaining in the queue.
    //
    if (gatherWriter.isOK()) {
        if (!msg_queue()->is_empty()) {
            msg_queue()->activate();
            while (1) {
                int remaining = getq(data);
                MessageManager mgr(data);
                if (!mgr.hasNativeMessageType(getMetaTypeInfoKey())) {
                    LOGFATAL << "invalid message type in queue - " << mgr.getMessageType() << std::endl;
                    ::abort();
                }
                gatherWriter.add(mgr.getEncoded());
                if (!remaining) break;
            }
            msg_queue()->deactivate();
        }
        gatherWriter.flush();
    } else {
        msg_queue()->deactivate();
    }

    ACE_OS::fsync(writer_.getDevice().get_handle());
    writer_.close();

    LOGDEBUG << "finished" << std::endl;
    return 0;
}
