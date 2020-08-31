#include <algorithm>
#include <functional>
#include <iterator>

#include "boost/bind/bind.hpp"

#include "TimeStampRangeSet.h"
#include "Utils/Utils.h"

using namespace SideCar::Time;
using namespace boost::placeholders;

void
TimeStampRangeSet::fill(const std::vector<std::string>& specs)
{
    std::for_each(specs.begin(), specs.end(), boost::bind(&TimeStampRangeSet::insert, this, _1));
}

bool
TimeStampRangeSet::contains(const TimeStamp& when) const
{
    const_iterator pos = std::find_if(begin(), end(), boost::bind(&TimeStampRange::contains, _1, when));
    return pos != end();
}
