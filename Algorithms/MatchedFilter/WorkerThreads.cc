#include "Algorithms/Algorithm.h"
#include "Logger/Log.h"

#include "WorkRequest.h"
#include "WorkRequestQueue.h"
#include "WorkerThreads.h"

using namespace SideCar::Algorithms::MatchedFilterUtils;

Logger::Log&
WorkerThreads::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.Algorithms.MatchedFilter.WorkerThreads");
    return log_;
}

WorkerThreads::WorkerThreads(Algorithm& algorithm,
                             WorkRequestQueue& pendingQueue,
                             WorkRequestQueue& finishedQueue)
    : Super(0), algorithm_(algorithm), pendingQueue_(pendingQueue),
      finishedQueue_(finishedQueue)
{
    Logger::ProcLog log("WorkerThreads", Log());
    LOGINFO << std::endl;
}

WorkerThreads::~WorkerThreads()
{
    Logger::ProcLog log("~WorkerThreads", Log());
    LOGINFO << std::endl;
}

int
WorkerThreads::svc()
{
    Logger::ProcLog log("svc", Log());
    LOGINFO << "starting" << std::endl;

    // Fetch the next WorkRequest object from the pending work request queue, process it, and then add it to the
    // finished queue.
    //
    ACE_Message_Block* data = 0;
    while (pendingQueue_.dequeue_head(data) != -1) {
	WorkRequest::FromMessageBlock(data)->process();
	finishedQueue_.enqueue_tail(data);
    }

    LOGWARNING << "exiting" << std::endl;

    return 0;
}
