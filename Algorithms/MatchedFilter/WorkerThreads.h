#ifndef SIDECAR_ALGORITHMS_MATCHEDFILTER_WORKERTHREAD_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MATCHEDFILTER_WORKERTHREAD_H

#include "ace/Task.h"

#include "WorkRequest.h"

namespace SideCar {
namespace Algorithms {

class Algorithm;

namespace MatchedFilterUtils {

/** Thread pool that generates threads that perform the following in the overridden svc() method:

    - fetch the next WorkRequest object from the pending WorkRequest queue
    - call the WorkRequest::process() method to process a slice of the data
    - post the finished WorkRequest object onto the finished WorkRequest queue
*/
class WorkerThreads : public ACE_Task<ACE_MT_SYNCH> {
    using Super = ACE_Task<ACE_MT_SYNCH>;

public:
    /** Log device for WorkerThreads objects

        \return Logger::Log device reference
    */
    static Logger::Log& Log();

    /** Constructor for the thread pool. Installs the pending and finished WorkRequest object queues. NOTE: does
        not create any threads; one must still invoke the ACE_Task::activate() method.

        \param algorithm the thread will post output messages to the
        Algorithm::send() method of this object

        \param pendingQueue thread-safe FIFO queue for pending WorkRequest
        objects

        \param finishedQueue thread-safe FIFO queue for completed WorkRequest
        objects
    */
    WorkerThreads(Algorithm& algorithm, WorkRequestQueue& pendingQueue, WorkRequestQueue& finishedQueue);

    /** Destructor. Here only to emit a log message for debugging.
     */
    ~WorkerThreads();

    /** Override of ACE_Task method. Contains the code that runs in a separate thread. When the routine returns,
        the thread stops.

        \return always 0.
    */
    int svc();

private:
    Algorithm& algorithm_;
    WorkRequestQueue& pendingQueue_;
    WorkRequestQueue& finishedQueue_;
};

} // namespace MatchedFilterUtils
} // end namespace Algorithms
} // end namespace SideCar

#endif
