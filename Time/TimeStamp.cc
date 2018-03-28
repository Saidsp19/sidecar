#include "QtCore/QRegExp"
#include "QtCore/QString"
#include "QtCore/QStringList"

#include <cmath>
#include <errno.h>
#include <iomanip>
#include <sstream>

#include "Logger/Log.h"

#include "TimeStamp.h"

using namespace SideCar::Time;

double const TimeStamp::kSecondsPerMicro = 1.E-6;
suseconds_t const TimeStamp::kMicrosPerSecond = ACE_ONE_SECOND_IN_USECS;

const TimeStamp&
TimeStamp::Min()
{
    static TimeStamp min_(0, 0);
    return min_;
}

const TimeStamp&
TimeStamp::Max()
{
    static TimeStamp max_(std::numeric_limits<int>::max(), kMicrosPerSecond - 1);
    return max_;
}

TimeStamp
TimeStamp::Now()
{
    ::timeval tv;
    ::gettimeofday(&tv, 0);
    return TimeStamp(tv);
}

TimeStamp
TimeStamp::ParseSpecification(const std::string& spec, const TimeStamp& zero)
{
    Logger::ProcLog log("ParseSpecification", Log());
    LOGINFO << spec << ' ' << zero << std::endl;

    if (!spec.size()) { throw InvalidSpecification("empty specification"); }

    // +[MM:]SS.SSS or SS:UU
    //
    QString z = QString::fromStdString(spec);
    QRegExp re("([+]?)((\\d+):)?((\\d+)(\\.\\d+)?)");

    LOGDEBUG << "re.isValid: " << re.isValid() << std::endl;
    LOGDEBUG << "indexIn: " << re.indexIn(z) << std::endl;
    LOGDEBUG << "matchedLength: " << re.matchedLength() << std::endl;

    if (!re.exactMatch(z)) {
        InvalidSpecification err("invalid specification - ");
        err << spec;
        throw err;
    }

    double seconds = 0.0;
    QStringList bits = re.capturedTexts();

    LOGDEBUG << "1: '" << bits[1].toStdString() << "' 2: '" << bits[2].toStdString() << "' 3: '"
             << bits[3].toStdString() << "' 4: '" << bits[4].toStdString() << "' 5: '" << bits[5].toStdString() << "'"
             << std::endl;

    TimeStamp rc;

    // If absolute time, extract the seconds and microseconds values and use to build a TimeStamp value.
    //
    if (bits[1].isEmpty()) {
        LOGDEBUG << "absolute time" << std::endl;
        long s = bits[3].toLong();
        long u = bits[5].toLong();
        rc = TimeStamp(s, u);
    } else {
        rc = zero;

        // Relative time here. Convert any minutes into seconds
        //
        if (!bits[3].isEmpty()) seconds += bits[3].toDouble() * 60.0;

        // Add seconds and fractional seconds to given base value.
        //
        if (!bits[4].isEmpty()) seconds += bits[4].toDouble();

        rc += seconds;
    }

    return rc;
}

Logger::Log&
TimeStamp::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("time.TimeStamp");
    return log_;
}

TimeStamp::TimeStamp(double when) : when_()
{
    static Logger::ProcLog log("TimeStamp(double)", Log());

    LOGDEBUG << when << std::endl;
    if (when > std::numeric_limits<long>::max()) {
        IllegalTimeStamp ex("TimeStamp::TimeStamp(double): value too large - ");
        log.thrower(ex << when);
    }

    when_.set(when);
}

ACE_InputCDR&
TimeStamp::load(ACE_InputCDR& cdr)
{
    int32_t seconds, micro;
    cdr >> seconds;
    cdr >> micro;
    when_.set(seconds, micro);
    return cdr;
}

ACE_OutputCDR&
TimeStamp::write(ACE_OutputCDR& cdr) const
{
    int32_t seconds = when_.sec();
    int32_t micro = when_.usec();
    cdr << seconds;
    cdr << micro;
    return cdr;
}

std::string
TimeStamp::hhmmss(int showMicroPlaces) const
{
    long secs = when_.sec();
    long hrs = secs / 3600;
    secs -= hrs * 3600;
    long mins = secs / 60;
    secs -= mins * 60;
    hrs = hrs % 24;

    std::ostringstream os("");
    os << std::fixed << std::setfill('0') << std::setw(2) << std::setprecision(2) << hrs << ':' << std::setw(2)
       << std::setprecision(2) << mins << ':';

    if (showMicroPlaces > 0) {
        os << std::setw(3 + showMicroPlaces) << std::setprecision(showMicroPlaces)
           << ((kSecondsPerMicro * when_.usec()) + secs);
    } else {
        os << std::setw(2) << std::setprecision(2) << secs;
    }

    return os.str();
}

time_t
TimeStamp::getSecondsRounded() const
{
    double value = asDouble();
    return ::lrint(value);
}
