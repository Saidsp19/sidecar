#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"

#include "Logger/Log.h"

#include "FileReaderTask.h"
#include "MessageManager.h"
#include "Module.h"
#include "ShutdownRequest.h"

using namespace SideCar::IO;

Logger::Log&
FileReaderTask::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.FileReaderTask");
    return log_;
}

FileReaderTask::Ref
FileReaderTask::Make()
{
    Ref ref(new FileReaderTask);
    return ref;
}

FileReaderTask::FileReaderTask()
    : Super(), reader_(), signalEndOfFile_(false), active_(false)
{
    ;
}

bool
FileReaderTask::openAndInit(const std::string& key, const std::string& path, bool signalEndOfFile,
                            long threadFlags, long threadPriority)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << "key: " << key << " path: " << path << " signalEndOfFile: " << signalEndOfFile << " threadFlags: "
            << std::hex << threadFlags << std::dec << " threadPriority: " << threadPriority << std::endl;

    threadFlags_ = threadFlags;
    threadPriority_ = threadPriority;
    if (! reactor()) reactor(ACE_Reactor::instance());

    if (! getTaskName().size()) {
	std::string taskName("FileReaderTask(");
	taskName += key;
	taskName += ',';
	taskName += path;
	taskName += ')';
	setTaskName(taskName);
    }

    if (key.size()) {
	setMetaTypeInfoKeyName(key);
    }

    signalEndOfFile_ = signalEndOfFile;
    ACE_FILE_Addr filePath(path.c_str());

    // Establish a connection to an actual device.
    //
    ACE_FILE_Connector connector;
    if (connector.connect(reader_.getDevice(),
                          filePath,
                          0,
                          ACE_Addr::sap_any,
                          0,
                          O_RDONLY,
                          ACE_DEFAULT_FILE_PERMS) == -1) {
	LOGERROR << "failed to open file " << path << std::endl;
	return false;
    }

    LOGINFO << "opened file " << path << std::endl;
    return true;
}

bool
FileReaderTask::enterRunState()
{
    if (active_) return true;
    return start();
}

bool
FileReaderTask::start()
{
    Logger::ProcLog log("start", Log());
    LOGINFO << "threadFlags: " << std::hex << threadFlags_ << std::dec << " threadPriority: " << threadPriority_
            << std::endl;

    active_ = true;
    if (activate(threadFlags_, 1, 0, threadPriority_) == -1) {
	active_ = false;
	LOGERROR << "failed to activate new thread - " << errno << std::endl;
	return false;
    }

    return true;
}

int
FileReaderTask::svc()
{
    static Logger::ProcLog log("svc", Log());
    LOGDEBUG << std::endl;

    // Keep running until told to stop
    //
    while (active_) {
	if (! reader_.fetchInput()) {
	    LOGWARNING << "EOF on file" << std::endl;
	    if (signalEndOfFile_) {
		ACE_Message_Block* data = ShutdownRequest().getWrapped();
		if (put_next(data) == -1) data->release();
	    }
	    break;
	}
	if (reader_.isMessageAvailable()) {
            LOGDEBUG << "got message" << std::endl;
	    acquireExternalMessage(reader_.getMessage());
        }
    }

    return 0;
}

int
FileReaderTask::close(u_long flags)
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << flags << std::endl;

    // Tell the reader thread to stop running, and then wait for the thread to quit.
    //
    if (flags) {
	if (active_) {
	    active_ = false;
	    wait();
	}
	reader_.close();
    }

    return Super::close(flags);
}

bool
FileReaderTask::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return put_next(data, timeout) != -1;
}
