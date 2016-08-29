#include <time.h>

#include "ClockSource.h"

using namespace Logger;

ClockSource::Ref
SystemClockSource::Make()
{
    ClockSource::Ref ref(new SystemClockSource);
    return ref;
}

void
SystemClockSource::now(timeval& t)
{
    ::gettimeofday(&t, 0);
}

ClockSource::Ref
DayClockSource::Make()
{
    ClockSource::Ref ref(new DayClockSource);
    return ref;
}

void
DayClockSource::now(timeval& t)
{
    ::gettimeofday(&t, 0);
    time_t secs = t.tv_sec;
    struct tm* parts = ::gmtime(&secs);
    t.tv_sec = parts->tm_sec + parts->tm_min * 60 + parts->tm_hour * 3600;
}
