#ifndef SIDECAR_IO_PROCESSINGSTAT_H	// -*- C++ -*-
#define SIDECAR_IO_PROCESSINGSTAT_H

#include <vector>

#include "Time/TimeStamp.h"
#include "Utils/RunningMedian.h"

namespace SideCar {
namespace Algorithms {

/** Processing statistic for an Algorithm. Records amount of time spent in algorithm-specific code, from the
    time just before entering its message displatch routines, to when it outputs a message. Maintains a vector
    of the last 1000 processing times, and returns their average.
*/
class ProcessingStat
{
public:

    /** Constructor.
     */
    ProcessingStat(size_t size = 1000) : orderedStats_(size) { reset(); }

    /** Reset the internal stat counters to zero.
     */
    void reset();

    /** Remember the current time for calculating processing times
     */
    void beginProcessing() { beginProcessing_ = Time::TimeStamp::Now(); }

    /** Calculate amount of time spent processing and add to the internal stat counters.
     */
    void endProcessing();

    void addSample(const Time::TimeStamp& delta) { orderedStats_.addValue(delta.asDouble()); }

    /** Obtain the current average processing time.

        \return average processing time
    */
    double getAverageProcessingTime() const { return orderedStats_.getEstimatedMeanValue(); }

    double getMedianProcessingTime() const { return orderedStats_.getMedianValue(); }

    double getMinimumProcessingTime() const { return orderedStats_.getMinimumValue(); }

    double getMaximumProcessingTime() const { return orderedStats_.getMaximumValue(); }

private:
    ::Utils::RunningMedian orderedStats_;
    Time::TimeStamp beginProcessing_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
