#include <algorithm>
#include <functional>
#include <iterator>
#include <sstream>

#include "TimeStampRange.h"

using namespace SideCar::Time;

TimeStampRange
TimeStampRange::Make(const std::string& spec)
{
    std::istringstream is(spec);
    double s, e;
    char dash;
    if (!(is >> s)) { throw std::runtime_error("invalid range specification"); }

    if (is.eof()) { return TimeStampRange(TimeStamp::Min(), TimeStamp(s)); }

    if (!(is >> dash) || dash != '-') { throw std::runtime_error("invalid range specification"); }

    if (!is.rdbuf()->in_avail()) { return TimeStampRange(TimeStamp(s), TimeStamp::Max()); }

    if (!(is >> e)) { throw std::runtime_error("invalid range specification"); }

    return TimeStampRange(TimeStamp(s), TimeStamp(e));
}

TimeStampRange::TimeStampRange(const TimeStamp& start, const TimeStamp& end) : start_(start), end_(end)
{
    if (start > end) {
        InvalidRange ex("start > end: ");
        ex << start << ' ' << end;
        throw ex;
    }
}
