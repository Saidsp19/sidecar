#ifndef LOGGER_CLOCKSOURCE_H	// -*- c++ -*-
#define LOGGER_CLOCKSOURCE_H

#include <sys/time.h>		// for timeval

#include "boost/shared_ptr.hpp"

namespace Logger {

/** Abstract base class for objects that can provide a timeval value. Derived classes must define a now()
    method. Used by a Log object to obtain the current time for a log message.
*/
class ClockSource
{
public:

    using Ref = boost::shared_ptr<ClockSource>;

    /** Destructor.
     */
    virtual ~ClockSource() {}

    /** Obtain the current time value from the clock.

        \param store reference to storage where the time will be written
    */
    virtual void now(timeval& store) = 0;
};

/** Concrete derivation of ClockSource that uses the Unix system clock to provide a timeval value.
 */
class SystemClockSource : public ClockSource
{
public:

    static Ref Make();

    /** Implementation of ClockSource API. Uses gettimeofday() to obtain a timeval value.
        
        \param store reference to storage where the time will be written
    */
    void now(timeval& store) override;

private:
    SystemClockSource() {}
};

/** Similar to SystemClockSource, but returns time values that range from 0 to 86400 seconds (24 hours).
 */
class DayClockSource : public ClockSource
{
public:

    static Ref Make();

    /** Implementation of ClockSource API. Uses gettimeofday() to obtain a timeval value, and then invokes
        gmtime() to obtain component bits of the timeval value.
        
        \param store reference to storage where the time will be written
    */
    void now(timeval& store) override;

private:
    DayClockSource() {}
};

} // end namespace Logger

/** \file
 */

#endif
