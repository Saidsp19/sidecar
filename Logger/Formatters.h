#ifndef LOGGER_FORMATTER_H	// -*- C++ -*-
#define LOGGER_FORMATTER_H

#include <iosfwd>
#include <stdexcept>		// for std::out_of_range

#include "boost/shared_ptr.hpp"

namespace Logger {

struct Msg;

/** Namespace for Log device formatters.
 */
namespace Formatters {

/** Abstract base class for objects wishing to act as a formatter for log messages. A formatter takes a Msg
    object and returns a std::string representation of the object.
*/
class Formatter
{
public:

    using Ref = boost::shared_ptr<Formatter>;
    
    /** Destructor.
     */
    virtual ~Formatter() {}

    /** Output a formattted log message

        \param os stream to write to

        \param msg log data to format
    */
    virtual void format(std::ostream& os, const Msg& msg) = 0;

protected:

    /** Constructor.
     */
    Formatter() {}
};

/** Log formatter that emits terse output.
 */
class Terse : public Formatter
{
public:

    using Ref = boost::shared_ptr<Terse>;
    static Ref Make() { Ref ref(new Terse); return ref; }

    /** Output a formattted log message

        \param os stream to write to

        \param msg log data to format
    */
    void format(std::ostream& os, const Msg& msg) override;

private:
    /** Constructor.
     */
    Terse() : Formatter() {}

};

/** Log formatter that emits verbose output.
 */
class Verbose : public Formatter
{
public:
    
    using Ref = boost::shared_ptr<Verbose>;
    static Ref Make() { Ref ref(new Verbose); return ref; }

    /** Output a formattted log message

        \param os stream to write to

        \param msg log data to format
    */
    void format(std::ostream& os, const Msg& msg) override;

private:

    /** Constructor.
     */
    Verbose() : Formatter() {}
};

/** Concrete Formatter class that uses a formatting pattern (ala \c printf and friends) to determine what
    components to put in the formatted output. A pattern consists of a text string, with one or more formatting
    flags. A formatting flag starts with a \c % character The following formatting flags may be used in a
    formatting pattern:
   
    - \c % insert a `%' character
    - \c c insert the log message category value
    - \c m insert the log message text
    - \c M insert the timestamp's microseconds value
    - \c p insert the log message priority name
    - \c P insert the log message priority value
    - \c S insert the timestamp's seconds value
    - \c w insert the timestamp of the log message. This uses the formatting defined by the TimeVal::operator<<()
    method for std::ostream streams.
    - \c W generate a text representation of the timestamp using a format similar to that of the Unix \c date
    utility 
    - \c ' take the formatting text between two single quotes and use as the format string for the strftime C
    library call. See the man page for strftime(3C).
    - \c z insert a '\n' character
*/
class Pattern : public Formatter
{
public:

    using Ref = boost::shared_ptr<Pattern>;
    static Ref Make() { Ref ref(new Pattern); return ref; }
    static Ref Make(const std::string& pattern) { Ref ref(new Pattern(pattern)); return ref; }

    /** Use a different formatting pattern.

        \param pattern formatting pattern to use for log messages.
    */
    void setPattern(const std::string& pattern) { pattern_ = pattern; }

    /** Build a std::string representation of a Msg object.

        \param msg object to format

        \param os stream to write to
    */
    void format(std::ostream& os, const Msg& msg) throw(std::out_of_range);

private:

    /** Constructor. Install default pattern which mimics VerboseFormatter.
     */
    Pattern() : Formatter(), pattern_("%w %p %c: %m") {}

    /** Constructor

        \param pattern formatting pattern to use for log messages
    */
    Pattern(const std::string& pattern) : Formatter(), pattern_(pattern) {} 

    std::string pattern_;	///< Formatting specification
};

}				// namespace Formatters
}				// namespace Logger

/** \file
 */

#endif
