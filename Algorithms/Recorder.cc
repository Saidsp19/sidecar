
#include "ace/FILE_Connector.h"

#include "IO/MessageManager.h"
#include "IO/GatherWriter.h"
#include "Logger/Log.h"
#include "Utils/Format.h"

#include "Recorder.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

Logger::Log&
Recorder::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.Algorithms.Recorder");
    return log_;
}

Recorder::Recorder(IO::Task& owner)
    : ACE_Task<ACE_MT_SYNCH>(), owner_(owner), writer_()
{
    msg_queue()->deactivate();
}

bool
Recorder::start(const std::string& path)
{
    Logger::ProcLog log("start", Log());
    LOGINFO << path << std::endl;

    // Attempt to establish open a writable connection to the given file path.
    //
    if (path.size() == 0) {
	LOGERROR << "NULL path for open" << std::endl;
	owner_.setError("Empty recording path");
	return false;
    }

    ACE_FILE_Addr addr(path.c_str());
    ACE_FILE_Connector connector;
    if (connector.connect(writer_.getDevice(),
                          addr,
                          0,		       // timeout
                          ACE_Addr::sap_any, // ignored
                          0,		       // ignored
                          O_WRONLY | O_CREAT | O_EXCL,
                          S_IRUSR | S_IWUSR |
                          S_IRGRP | S_IWGRP |
                          S_IROTH | S_IWOTH) == -1) {
	LOGERROR << "failed to open file for recording - "
		 << Utils::showErrno() << std::endl;
	owner_.setError("Failed to open recording file");
	return false;
    }

    // Ready to start our recording thread. Activate our incoming message queue so that the thread can use it.
    //
    msg_queue()->activate();

    long flags = THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED;
    long priority = 0;

    if (activate(flags, 1, 0, priority) == -1) {
	LOGERROR << "failed to spawn writer thread" << std::endl;
	owner_.setError("Failed to start writer thread");
	return false;
    }

    return true;
}

bool
Recorder::stop()
{
    Logger::ProcLog log("stop", Log());
    LOGINFO << std::endl;

    // Deactivate the message queue. This will stop the writing thread running the svc() method to stop and
    // exit. Don't return until the svc() thread has finished.
    //
    msg_queue()->deactivate();
    if (wait() == -1)
	LOGERROR << "failed wait() for writer thread - " << Utils::showErrno()
		 << std::endl;

    return true;
}

int
Recorder::svc()
{
    // !!! Running in a separate thread here -- use protection...
    //
    Logger::ProcLog log("svc", Log());
    LOGINFO << "starting" << std::endl;

    // Create a gather writer that will write out in 32K bursts. Should be a configurable parameter to allow for
    // testing.
    //
    IO::GatherWriter gatherWriter(writer_);
    gatherWriter.setSizeLimit(32 * 1024);

    // Main processing loop. Fetch from the input queue and add to a GatherWriter object. A failure to fetch
    // means that the queue has been deactivated by the producer thread as a signal for us to quit. A failure of
    // the GatherWriter probably means we ran out of space.
    //
    ACE_Message_Block* data = 0;
    while (gatherWriter.isOK() && getq(data) != -1) {
	LOGDEBUG << "msg: " << data << std::endl;
	IO::MessageManager mgr(data);
	if (! gatherWriter.add(mgr.getEncoded())) {
	    msg_queue()->deactivate();
	}
    }

    // If our GatherWriter is still OK, then the producer thread told to quit. Continue writing out anything in
    // our queue.
    //
    if (gatherWriter.isOK()) {

	// Flush anything remaining in the queue to disk. Its OK to reactivate now since at this point the
	// producer thread is waiting for us to exit and will not add anything to the queue.
	//
	if (! msg_queue()->is_empty()) {
	    msg_queue()->activate();
	    while (1) {
		int remaining = getq(data);
		IO::MessageManager mgr(data);
		gatherWriter.add(mgr.getEncoded());
		if (! remaining) break;
	    }
	    msg_queue()->deactivate();
	}
	gatherWriter.flush();
    }
    else {

	// Our gatherWriter has failed, so deactivate our queue so future puts will fail, which alert the
	// producer thread that something bad has happened.
	//
	msg_queue()->deactivate();
    }

    // Safe now to close the file connection. Use ::fsync to make sure we write everything to disk just to be
    // safe. This is to be POSIX compliant.
    //
    ACE_OS::fsync(writer_.getDevice().get_handle());
    writer_.getDevice().close();

    LOGINFO << "terminating" << std::endl;
    return 0;
}

bool
Recorder::isActive()
{
    return ! msg_queue()->deactivated();
}
