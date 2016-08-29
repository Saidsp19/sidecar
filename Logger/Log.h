#ifndef LOGGER_LOG_H		// -*- C++ -*-
#define LOGGER_LOG_H

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "boost/function.hpp"
#include "boost/shared_ptr.hpp"

#include "Logger/Priority.h"
#include "Logger/LogStreamBuf.h"
#include "Logger/Writers.h"
#include "Threading/Threading.h" // for Threading::Mutex

namespace Utils { class Exception; }
namespace Logger {

class ClockSource;
class LogMap;
class Msg;

/** Master class for the Logger system. A Log instance acts as a filter and destination for log messages created
    by the user. Each log message has an associated Priority value; if the message's priority is less than the
    Log instance's priority setting, the message is accepted for posting, and is then forwarded to the Writer
    objects registered for the Log instance. As a result, Log instances do not work directly with log messages,
    apart from filtering by priority level.

    NOTE: most methods below are <b>not</b> thread-safe. The sole exception are Find() and Root(), which rely on
    a mutex in MakeObj to guarantee that there is a one-to-one correlation between a log device name, and a Log
    object.
   
    Each Log instance has a parent, which is also a Log instance (the only exception to this is the top-level
    `root' Log instance.) A log message posted to a child will also propagate up the inheritance hierarchy to
    each parent.

    Log messages are sent to Writer objects that manage a particular device or file. For instance, there is a
    Writer object that will write log messages to a file, and a Writer object that talks to a syslog daemon.
    Each Writer object has an associated Formatter object which takes each log message (represented by a Msg
    object) and generates a textual representation of it.

    Log message streams are obtained by calling one of the Log stream-generating methods (fatal, error, warning,
    info, debug1). For instance, to send a debug-level message, one could do the following

    \code
    Log::Root().debug1() << "this is a debug message" << std::endl;
    \endcode

    There is only one output stream object for all Log objects of a given thread. The stream object accumulates
    message elements via the '<<' operator until it encounters an `endl' or `flush' manipulator.

    When the output stream encounters an std::endl token it calls the LogStreamBuf::sync() method of the
    stream's LogStreamBuf instance. This method invokes the #post() method of the assigned Log device, handing
    it the accumulated text and the priority level to post at. The #post() method in turn creates a Msg object
    to represent the text string, priority level, and time of posting, which is then passed to the postMsg()
    method for all Log objects in the parent hierarchy that accept the message's priority level. Finally,
    postMsg() gives the message to registered writers that do the actual writing of the log message to a log
    device.

    For the best performance, one should conditionalize log message generation with a call to one of the
    `shows*' methods (eg showsDebug() or showsError()).

    \code
    if (log.showsDebug1()) log.debug1() << "hi mom!" << std::endl;
    \endcode

    For convenience, there are macros that do just this:

    \code
    LOGDEBUG3
    LOGDEBUG2
    LOGDEBUG1 (LOGDEBUG)
    LOGENTER
    LOGEXIT
    LOGINFO
    LOGWARNING
    LOGERROR
    LOGFATAL
    \endcode

    Using these macros will result in the best performance regardless of log priority level settings. However,
    they do assume that there is a variable named `log' that refers to a Log instance, such as that returned by
    the Log::Find() method:

    \code
    Logger::Log& log = Logger::Log::Find("a.b.c");
    LOGDEBUG << "hi mom!" << std::endl;
    \endcode
*/
class Log
{
public:

    /** Change the source of timestamps used in messages. NOTE: not thread-safe!

	\param clock ClockSource to install

	\return previous clock source object
    */
    using ClockSourceRef = boost::shared_ptr<ClockSource>;
    static ClockSourceRef SetClockSource(const ClockSourceRef& clock);

    /** Get the source of timestamps used in messages. If there is not a clock source available, a
	SystemClockSource object will be created and returned. NOTE: not thread-safe!

        \return active clock source
    */
    static ClockSourceRef GetClockSource();

    /** Class method that locates/creates a Log instance under a given name. The name consists of zero or more
	parent names, separated by `.' characters. If there are no parent names, the Log instance is assigned as
	a child to the top-level `root' object.
        
	\param name the name to search for

	\param hidden if true and a new log device is created, it will not appear in the output of GetNames()

	\return Log instance found/created
    */
    static Log& Find(const std::string& name, bool hidden = false);

    /** Class method that locates/creates the top-level Log instance.

	\return top-level Log instance
    */
    static Log& Root();

    using NewLogNotifier = boost::function<void(Log&)>;

    /** Obtain a vector of all the currently known Log device names (these are the full, dot-separated values).

        \return known Log names
    */
    static std::vector<std::string> GetNames();

    /** Obtain a vector of all the currently known Log device names (these are the full, dot-separated values),
        and register a notifier to be called when new Log devices are encountered.

        \return known Log names
    */
    static std::vector<std::string> GetNames(NewLogNotifier observer);

    /** Create a proper log path name

        \param prefix the prefix of the full name

        \param tail the tail of the full name

        \return new name
    */
    static std::string MakeFullName(const std::string& prefix, const std::string& tail);

    /** Destructor.
     */
    ~Log();

    /** Obtain name of the log device. Does not contain names of parents.

        \return short device name
    */
    const std::string& name() const { return name_; }

    /** Obtain fully-qualified name of the log device. This includes the names of parents.

        \return complete device name
    */
    const std::string& fullName() const { return fullName_; }

    /** Assign a new priority level to the Log instance

        \param priority new level to use
    */
    void setPriorityLimit(Priority::Level priority);

    /** Set whether message that are emitted by this device are also given to its parent for posting. The
        default for new Log devices is true.

        \param propagate true if propagating
    */
    void setPropagate(bool propagate) { propagate_ = propagate; }

    /** Determine if the log is visible in the list of names from GetNames()

        \return current value
    */
    bool isHidden() const { return hidden_; }

    /** Obtain the number of children below this log level

        \return number of children
    */
    size_t getNumChildren() const { return children_.size(); }

    /** Obtain the child at the given index.

        \param index child to return

        \return Log object
    */
    Log* getChild(size_t index) const { return children_[index]; }

    /** Obtain current priority limit of log device.

	\return priority level
    */
    Priority::Level getPriorityLimit() const { return priorityLimit_; }

    /** Obtain max priority limit of the log device, which is defined as the max of getPriorityLimit() and
	getParent().getMaxPriorityLimit().

	\return priority level
    */
    Priority::Level getMaxPriorityLimit() const { return maxPriorityLimit_; }

    /** Determine if this Log instance is accepting a given Priority level.

        \param priority priority level to check for

	\return true if accepted
    */
    bool isAccepting(Priority::Level priority) const { return priority <= maxPriorityLimit_; }

    /** Obtain the parent of this log device.

	\return parent Log object, or NULL if this is the root Log device.
    */
    Log* getParent() const { return parent_; }

    /** Add a new Writer instance to the set of log message writers.

	\param writer instance to add
    */
    void addWriter(const Writers::Writer::Ref& writer);

    /** Remove a previously-added Writer instance from the set of log message writers.

        \param writer instance to remove
    */
    void removeWriter(const Writers::Writer::Ref& writer);

    /** Remove all Writer objects associated with this Log instance.
     */
    void removeAllWriters();

    /** \return the number of registered Writer objects
     */
    int numWriters() const;

    /** Ask all registered writers for the log device to write out any unwritten log messages.
     */
    void flushWriters();

    /** Submit a log message to all of the assigned LogWriters. Does nothing if the given message level is
	greater than the priority setting of this instance or any parent priority settings.
        
	\param priority severity level of the log message

	\param msg text of the log message
    */
    void post(Priority::Level priority, const std::string& msg) const;

    /** Determine if a `fatal' log message would reach a log device.
        
	\return true if so
    */
    bool showsFatal() const { return isAccepting(Priority::kFatal); }

    /** Determine if a `error' log message would reach a log device.
        
	\return true if so
    */
    bool showsError() const { return isAccepting(Priority::kError); }

    /** Determine if a `warning' log message would reach a log device.
        
	\return true if so
    */
    bool showsWarning() const { return isAccepting(Priority::kWarning); }

    /** Determine if an 'info' log message would reach a log device.
        
	\return true if so
    */
    bool showsInfo() const { return isAccepting(Priority::kInfo); }
    
    /** Determine if an 'traceIn' log message would reach a log device.
        
	\return true if so
    */
    bool showsTraceIn() const { return isAccepting(Priority::kTraceIn); }
    
    /** Determine if an 'traceOut' log message would reach a log device.
        
	\return true if so
    */
    bool showsTraceOut() const { return isAccepting(Priority::kTraceOut); }
    
    /** Determine if a 'debug1' log message would reach a log device.
        
	\return true if so
    */
    bool showsDebug1() const { return isAccepting(Priority::kDebug1); }

    /** Determine if a 'debug2' log message would reach a log device.
        
	\return true if so
    */
    bool showsDebug2() const { return isAccepting(Priority::kDebug2); }

    /** Determine if a 'debug3' log message would reach a log device.
        
	\return true if so
    */
    bool showsDebug3() const { return isAccepting(Priority::kDebug3); }

    /** Obtain an output stream to use for a message of a certain priority level. All Log instances in the same
	thread share the same output stream. If no log device would write out a message at the given priority
	level, an `null' output stream is returned, one that is setup to not do any writing.

        \param level priority level of the next log message written to the stream.

	\return output stream
    */
    std::ostream& getStream(Priority::Level priority);

    /** Obtain output stream for `fatal' log messages.

        \return output stream
    */
    std::ostream& fatal() { return getStream(Priority::kFatal); }

    /** Obtain output stream for `error' log messages.

	\return output stream
    */
    std::ostream& error() { return getStream(Priority::kError); }

    /** Obtain output stream for `warning' log messages.

	\return output stream
    */
    std::ostream& warning() { return getStream(Priority::kWarning); }

    /** Obtain output stream for `info' log messages.

	\return output stream
    */
    std::ostream& info() { return getStream(Priority::kInfo); }

    /** Obtain output stream for `trace' log messages.

	\return output stream
    */
    std::ostream& traceIn() { return getStream(Priority::kTraceIn); }

    /** Obtain output stream for `trace' log messages.

	\return output stream
    */
    std::ostream& traceOut() { return getStream(Priority::kTraceOut); }

    /** Obtain output stream for `debug1' log messages.

	\return output stream
    */
    std::ostream& debug1() { return getStream(Priority::kDebug1); }

    /** Obtain output stream for `debug2' log messages.

	\return output stream
    */
    std::ostream& debug2() { return getStream(Priority::kDebug2); }

    /** Obtain output stream for `debug3' log messages.

	\return output stream
    */
    std::ostream& debug3() { return getStream(Priority::kDebug3); }

    /** Generate a `fatal' log message from a exception object, and throw the exception object. As such, it will
	not return...

        \param ex exception to log and throw
    */
    template <typename T>
    void thrower(const T& ex) { fatal() << ex.err() << std::endl; throw ex; }

private:

    /** Constructor. Only visible to the FindObj class method, which creates new Log instances.
        
	\param name name of the Log instance

	\param fullName full name of the Log instance, including the parent
	hierarchy.

	\param parent Log instance that is the parent to this one. Only the
	top-level `root' instance has NULL for this value.

	\param priority Priority level used to limit which log messages are
	posted.
    */
    Log(const std::string& name, const std::string& fullName, Log* parent, Priority::Level priority, bool hidden);

    /** Copy constructor. Prohibited.
     */
    Log(const Log&);

    /** Assignment operator. Prohibited.
     */
    Log& operator =(const Log&);

    void initialize();

    /** Private method used by the Log::post to send the log message to registered Writer instances.
        
	\param msg Msg instance to send
    */
    void postMsg(const Msg& msg) const;

    void updateMaxPriorityLimit(Priority::Level maxPriority);

    /** Max level of log messages that are accepted for posting.
     */
    Priority::Level priorityLimit_;
    Priority::Level maxPriorityLimit_;

    /** Parent of this log device. If this is the top-level device, then this will be nullptr.
     */
    Log* parent_;

    /** Short name of the log device.
     */
    std::string name_;

    /** Full-name of the log device.
     */
    std::string fullName_;

    bool propagate_;

    bool hidden_;

    /** Collection of writers assigned to this log device. Any changes to this container should be protected by
        first acquiring the writersMutex_ below.
    */
    using WritersSet = std::set<Writers::Writer::Ref>;
    WritersSet writers_;

    /** Access lock for the writers_ and streams_ containers.
     */
    mutable Threading::Mutex::Ref modifyMutex_;

    std::vector<Log*> children_;

    /** Class method used to locate/create unique Log instances. If a Log object under the given name does not
        already exist, one will be created.

        \param name name of the Log instance to locate or create

        \return found Log object
    */
    static Log* FindObj(const std::string& name, bool hidden);

    /** Create the root object. Install a formatter and log writer using std::cerr.

        \return 
    */
    static Log* MakeRoot();

    /** Create a new Log object.

        \param fullName name indicating hierarchy of log device

        \param objName leaf name for the log device

        \param parent ancestor Log device

        \return new log device
    */
    static Log* MakeObj(const std::string& name, const std::string& fullName, Log* parent, bool hidden);

    friend class LogMap;
};

/** Utility class used to create log messages prefixed with a procedure name. To use, simply create an instance
    and use in place of a Log instance. To minimize routine entry times, one can create a static instance like
    so:

    \code
    void Foo::bar() {
    static Logger::ProcLog log("bar", Log());
    log.debug() << "hi mom!" << std::endl;
    \endcode

    The above assumes that the class Foo defined a (class) method called Log() which returned the general log
    device to use for all Foo-related log messages.
*/
class ProcLog
{
public:

    /** Constructor.

        \param name routine name to prepend to log messages

	\param log device to send log messages to
    */
    ProcLog(const char* name, Log& log);

    /** Constructor.

        \param name routine name to prepend to log messages

	\param log device to send log messages to
    */
    ProcLog(const std::string& name, Log& log);

    /** Generate a `fatal' log message from an Utils::Exception object, and throw the exception object. As such,
	it will not return...

        \param ex exception to log and throw
    */
    template <typename T>
    void thrower(const T& ex) { log_.fatal() << ex.err() << std::endl; throw ex; }

    /** Determine if a 'fatal' log message would reach a log device.

	\return true if so
    */
    bool showsFatal() const { return log_.showsFatal(); }

    /** Determine if an 'error' log message would reach a log device.

        \return true if so
    */
    bool showsError() const { return log_.showsError(); }

    /** Determine if a 'warning' log message would reach a log device.

        \return true if so
    */
    bool showsWarning() const { return log_.showsWarning(); }

    /** Determine if an 'info' log message would reach a log device.

        \return true if so
    */
    bool showsInfo() const { return log_.showsInfo(); }

    /** Determine if a 'traceIn' log message would reach a log device.

        \return true if so
    */
    bool showsTraceIn() const { return log_.showsTraceIn(); }

    /** Determine if a 'traceOut' log message would reach a log device.

        \return true if so
    */
    bool showsTraceOut() const { return log_.showsTraceOut(); }

    /** Determine if a 'debug' log message would reach a log device.

        \return true if so
    */
    bool showsDebug1() const { return log_.showsDebug1(); }

    /** Determine if a 'debug' log message would reach a log device.

        \return true if so
    */
    bool showsDebug2() const { return log_.showsDebug2(); }

    /** Determine if a 'debug' log message would reach a log device.

        \return true if so
    */
    bool showsDebug3() const { return log_.showsDebug3(); }

    /** Obtain output stream for `fatal' log messages.

        \return output stream
    */
    std::ostream& fatal() { return log_.fatal(); }

    /** Obtain output stream for `error' log messages.

        \return output stream
    */
    std::ostream& error() { return log_.error(); }

    /** Obtain output stream for `warning' log messages.

        \return output stream
    */
    std::ostream& warning() { return log_.warning(); }

    /** Obtain output stream for `info' log messages.

        \return output stream
    */
    std::ostream& info() { return log_.info(); }

    /** Obtain output stream for `trace' log messages.

        \return output stream
    */
    std::ostream& traceIn() { return log_.traceIn(); }

    /** Obtain output stream for `trace' log messages.

        \return output stream
    */
    std::ostream& traceOut() { return log_.traceOut(); }

    /** Obtain output stream for `debug1' log messages.

        \return output stream
    */
    std::ostream& debug1() { return log_.debug1(); }

    /** Obtain output stream for `debug2' log messages.

        \return output stream
    */
    std::ostream& debug2() { return log_.debug2(); }

    /** Obtain output stream for `debug3' log messages.

        \return output stream
    */
    std::ostream& debug3() { return log_.debug3(); }

protected:
    Log& log_;			///< device to use for log messages
};

/** Macro to conditionally obtain a log stream if log device is accepting debug3 messages.
 */
#define LOGDEBUG3 if (log.showsDebug3()) log.debug3()

/** Macro to conditionally obtain a log stream if log device is accepting debug2 messages.
 */
#define LOGDEBUG2 if (log.showsDebug2()) log.debug2()

/** Macro to conditionally obtain a log stream if log device is accepting debug1 messages.
 */
#define LOGDEBUG1 if (log.showsDebug1()) log.debug1()

/** Macro to conditionally obtain a log stream if log device is accepting debug1 messages.
 */
#define LOGDEBUG if (log.showsDebug1()) log.debug1()

/** Macro to conditionally obtain a log stream if log device is accepting trace messages.
 */
#define LOGTOUT if (log.showsTraceOut()) log.traceOut()

/** Macro to conditionally obtain a log stream if log device is accepting trace messages.
 */
#define LOGTIN if (log.showsTraceIn()) log.traceIn()

/** Macro to conditionally obtain a log stream if log device is accepting info messages.
 */
#define LOGINFO if (log.showsInfo()) log.info()

/** Macro to conditionally obtain a log stream if log device is accepting warning messages.
 */
#define LOGWARNING if (log.showsWarning()) log.warning()

/** Macro to conditionally obtain a log stream if log device is accepting error messages.
 */
#define LOGERROR if (log.showsError()) log.error()

/** Macro to conditionally obtain a log stream if log device is accepting fatal messages.
 */
#define LOGFATAL if (log.showsFatal()) log.fatal()

} // namespace Logger

/** \file
 */

#endif
