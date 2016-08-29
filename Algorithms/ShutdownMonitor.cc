#include "ace/Reactor.h"

#include "IO/MessageManager.h"
#include "IO/Stream.h"
#include "Logger/Log.h"

#include "ShutdownMonitor.h"

using namespace SideCar::Algorithms;

Logger::Log&
ShutdownMonitor::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Algorithms.ShutdownMonitor");
    return log_;
}

ShutdownMonitor::Ref
ShutdownMonitor::Make()
{
    Ref ref(new ShutdownMonitor);
    return ref;
}

ShutdownMonitor::ShutdownMonitor()
    : IO::Task(), staleCount_(0)
{
    setTaskName("ShutdownMonitor");
    reactor(ACE_Reactor::instance());
}

int
ShutdownMonitor::close(u_long flags)
{
    reactor()->cancel_timer(this);
    return 0;
}

bool
ShutdownMonitor::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return put_next(data, timeout) != -1;
}

bool
ShutdownMonitor::doShutdownRequest()
{
    Logger::ProcLog log("doShutdownRequest", Log());
    LOGINFO << std::endl;
    ACE_Time_Value delay(2);
    return reactor()->schedule_timer(this, 0, delay, delay) != -1;
}

int
ShutdownMonitor::handle_timeout(const ACE_Time_Value& now, const void* act)
{
    static Logger::ProcLog log("handle_timeout", Log());
    LOGINFO << std::endl;

    int index = 0;
    while (1) {
        IO::Stream::Ref stream = getStream().lock();
	IO::Task::Ref task;
        if (stream) task = stream->getTask(index);

	if (! task) {
	    if (++staleCount_ == 5)
		reactor()->end_event_loop();
	    return 0;
	}
	else if (task->msg_queue()->message_count() > 0) {
	    LOGWARNING << "task " << index << " has pending data" << std::endl;
	    staleCount_ = 0;
	    return 0;
	}

	++index;
    }

    return 0;
}
