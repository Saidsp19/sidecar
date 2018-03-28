#ifndef SIDECAR_ALGORITHMS_MATCHEDFILTERUTILS_WORKREQUESTQUEUE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MATCHEDFILTERUTILS_WORKREQUESTQUEUE_H

#include "ace/Condition_Thread_Mutex.h"
#include "ace/Message_Queue_T.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Algorithms {
namespace MatchedFilterUtils {

class WorkRequestQueue : public ACE_Message_Queue<ACE_MT_SYNCH> {
public:
    static Logger::Log& Log();

    WorkRequestQueue(const std::string& tag);

    ~WorkRequestQueue();

private:
    std::string tag_;
};

} // end namespace MatchedFilterUtils
} // end namespace Algorithms
} // end namespace SideCar

#endif
