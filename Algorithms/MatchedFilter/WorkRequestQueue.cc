#include "Logger/Log.h"

#include "WorkRequestQueue.h"

using namespace SideCar::Algorithms::MatchedFilterUtils;

Logger::Log&
WorkRequestQueue::Log()
{
    static Logger::Log& log_ = Logger::Log::Find(
	"SideCar.Algorithms.MatchedFilter.WorkRequestQueue");
    return log_;
}

WorkRequestQueue::WorkRequestQueue(const std::string& tag)
    : ACE_Message_Queue<ACE_MT_SYNCH>(), tag_(tag)
{
    Logger::ProcLog log("WorkRequestQueue", Log());
    LOGINFO << tag_ << " created" << std::endl;
}

WorkRequestQueue::~WorkRequestQueue()
{
    Logger::ProcLog log("~WorkRequestQueue", Log());
    LOGINFO << tag_ << " destroying - isEmpty:" << is_empty()
	    << std::endl;
}
