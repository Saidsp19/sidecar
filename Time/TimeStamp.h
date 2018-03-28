#ifndef SIDECAR_TIME_TIMESTAMP_H // -*- C++ -*-
#define SIDECAR_TIME_TIMESTAMP_H

#include "ace/Time_Value.h"
#include "boost/operators.hpp"

#include "IO/CDRStreamable.h"
#include "IO/Printable.h"
#include "Utils/Exception.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Time {

/** Representation of a SideCar time value. Internally, uses ACE_Time_Value to store a time value.
 */
class TimeStamp : public boost::totally_ordered<TimeStamp>,
                  public boost::additive<TimeStamp>,
                  public IO::CDRStreamable<TimeStamp>,
                  public IO::Printable<TimeStamp> {
public:
    static double const kSecondsPerMicro;
    static suseconds_t const kMicrosPerSecond;

    struct IllegalTimeStamp : public Utils::Exception, public Utils::ExceptionInserter<IllegalTimeStamp> {
        IllegalTimeStamp(const std::string& err) : Utils::Exception(err) {}
    };

    struct InvalidSpecification : public Utils::Exception, public Utils::ExceptionInserter<InvalidSpecification> {
        InvalidSpecification(const std::string& err) : Utils::Exception(err) {}
    };

    /** Obtain representation of minimum value that TimeStamp can represent. This is just 0 seconds.

        \return reference to minimum value
    */
    static const TimeStamp& Min();

    /** Obtain representation of maximum value that TimeStamp can represent.

        \return reference to maximum value
    */
    static const TimeStamp& Max();

    /** Obtain a TimeStamp the contains the current time from the system clock.

        \return TimeStamp with current time
    */
    static TimeStamp Now();

    /** Parse a time specification and return the TimeStamp value it represents. Empty and invalid
        specifications throw InvalidSpecification errors. Supports relative and absolute specifications.
        Absolute times contains seconds and microseconds integer values, separated by a colon (':') character.
        Relative times start with a plus ('+') character followed by the number of seconds in the offset. This
        may be specified as a floating-point number, or as minutes ':' seconds. The resulting offset value is
        then added to the given 'zero' value and and returned.

        \param spec the time specification to parse

        \param zero the reference to use for relative times

        \return new TimeStamp value
    */
    static TimeStamp ParseSpecification(const std::string& spec, const TimeStamp& zero);

    /** Get the log device for TimeStamp log messages.

        \return Log reference
    */
    static Logger::Log& Log();

    /** Default constructor. Initializes value to TimeStamp::Min().
     */
    TimeStamp() : when_(0, 0) {}

    /** Conversion constructor for double values.

        \param value initial value to use
    */
    TimeStamp(double value);

    /** Constructor for integer seconds and microseconds

        \param seconds number of seconds since T0

        \param micro number of microseconds since T0
    */
    TimeStamp(time_t seconds, suseconds_t micro) : when_(seconds, micro) {}

    /** Conversion constructor for POSIX timeval members

        \param value initial value
    */
    TimeStamp(const ::timeval& value) : when_(value) {}

    /** Obtain the number of seconds in the time stamp

        \return seconds
    */
    time_t getSeconds() const { return when_.sec(); }

    /** Obtain the number of seconds in the time stamp, rounded by the value in the microseconds.

        \return seconds
    */
    time_t getSecondsRounded() const;

    /** Obtain the number of microseconds since T0 we represent

        \return microseconds
    */
    suseconds_t getMicro() const { return when_.usec(); }

    /** Obtain internal value as a C++ double.

        \return double representation
    */
    double asDouble() const { return getSeconds() + getMicro() * kSecondsPerMicro; }

    /** Allow object to appear where ACE_Time_Value objects may appear.

        \return ACE_Time_Value value.
    */
    operator const ACE_Time_Value&() const { return when_; }

    operator timeval() const { return when_; }

    std::string hhmmss(int showMicroPlaces = 0) const;

    bool operator==(const TimeStamp& rhs) const { return when_ == rhs.when_; }

    bool operator<(const TimeStamp& rhs) const { return when_ < rhs.when_; }

    /** Determine if the held time is not the smallest time value.

        \return true if so.
    */
    operator bool() const { return *this != Min(); }

    /** Determine if the held time is the smallest time value.

        \return true if so.
    */
    bool operator!() const { return *this == Min(); }

    /** Write out a textual representation of our time stamp.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const { return os << when_.sec() << ':' << when_.usec(); }

    /** Add the seconds of another time value to this one.

        \param rhs value to add

        \return reference to self
    */
    TimeStamp& operator+=(const TimeStamp& rhs)
    {
        when_ += rhs.when_;
        return *this;
    }

    /** Subtract the seconds of another time value from this one.

        \param rhs value to subtract

        \return reference to self
    */
    TimeStamp& operator-=(const TimeStamp& rhs)
    {
        when_ -= rhs.when_;
        return *this;
    }

    /** Multiply held time value by a scaling factor.

        \param scale value to multiply with

        \return reference to self
    */
    TimeStamp& operator*=(double scale)
    {
        when_ *= scale;
        return *this;
    }

    /** Divide the held time value by a scaling factor.

        \param scale value to divide with

        \return reference to self
    */
    TimeStamp& operator/=(double scale)
    {
        when_ *= (1.0 / scale);
        return *this;
    }

    /** Acquire a new value from a input CDR stream

        \param cdr stream to read from

        \return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr);

    /** Write out our value to an output CDR stream

        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const;

protected:
    TimeStamp(const ACE_Time_Value& when) : when_(when) {}

private:
    ACE_Time_Value when_;
};

} // end namespace Time
} // end namespace SideCar

/** \file
 */

#endif
