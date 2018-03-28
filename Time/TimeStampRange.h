#ifndef SIDECAR_TIME_TIMESTAMPRANGE_H // -*- C++ -*-
#define SIDECAR_TIME_TIMESTAMPRANGE_H

#include <string>

#include "Time/TimeStamp.h"
#include "Utils/Exception.h"

namespace SideCar {
namespace Time {

/** Representation of a span of time. Use the TimeStampRange::Make() class method to convert string range
    specifications into TimeStampRange objects. Use TimeStampRange::contains() to check if a TimeStamp is within
    a given range. Defines the less-than operator so TimeStampRange instances may be stored in ordered
    containers, such as std::set or std::map. The class TimeStampRangeSet is an enhanced std:set container for
    TimeStampRanges.
*/
class TimeStampRange {
public:
    /** Factory method that converts a range specification in a string into a TimeStampRange object. A
        specification may be in one of the following formats: - START-END - create a range of spanning START and
        END times - END - create a range from TimeStamp::Min() to END - START- - create a range from START to
        TimeStamp::Max() If the format is invalid, this routine throws a std::runtime_error exception.

        \param spec specification to parse

        \return TimeStampRange instance
    */
    static TimeStampRange Make(const std::string& spec);

    /** Exception thrown if a range is invalid (start > end).
     */
    struct InvalidRange : public Utils::Exception, public Utils::ExceptionInserter<InvalidRange> {
        InvalidRange(const std::string& err) : Utils::Exception(err), Utils::ExceptionInserter<InvalidRange>() {}
    };

    /** Constructor.

        \param start start of the time range

        \param end end of the time range
    */
    TimeStampRange(const TimeStamp& start, const TimeStamp& end);

    /** Less-than order operator. A range is considered "smaller" than another one if its start time is less
        than the other, or the start times are the same, but the end time is less than the other.

        \param rhs instance to compare with

        \return true if less than given value
    */
    bool operator<(const TimeStampRange& rhs) const
    {
        return start_ < rhs.start_ || (start_ == rhs.start_ && end_ < rhs.end_);
    }

    /** Obtain the start time of the range.

        \return TimeStamp reference
    */
    const TimeStamp& getStart() const { return start_; }

    /** Obtain the end time of the range.

        \return TimeStamp reference
    */
    const TimeStamp& getEnd() const { return end_; }

    /** Determine if a given TimeStamp value falls between the start end end times of a range, inclusive.

        \param when value to check

        \return true if contained
    */
    bool contains(const TimeStamp& when) const { return when >= start_ && when <= end_; }

private:
    TimeStamp start_; ///< Beginning of the range
    TimeStamp end_;   ///< End of the range
};

} // end namespace Time
} // end namespace SideCar

/** \file
 */

#endif
