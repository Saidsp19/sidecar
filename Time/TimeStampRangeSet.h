#ifndef SIDECAR_TIME_TIMESTAMPRANGESET_H // -*- C++ -*-
#define SIDECAR_TIME_TIMESTAMPRANGESET_H

#include <set>
#include <string>
#include <vector>

#include "Time/TimeStampRange.h"

namespace SideCar {
namespace Time {

class TimeStampRangeSet : public std::set<TimeStampRange>
{
    using Base = std::set<TimeStampRange>;
    
public:
    TimeStampRangeSet() : std::set<TimeStampRange>() {}

    TimeStampRangeSet(const std::vector<std::string>& specs) : std::set<TimeStampRange>() { fill(specs); }

    std::pair<iterator,bool> insert(const std::string& spec) { return Base::insert(TimeStampRange::Make(spec)); }

    void fill(const std::vector<std::string>& strings);

    bool contains(const TimeStamp& when) const;
};

} // end namespace Time
} // end namespace SideCar

/** \file
 */

#endif
