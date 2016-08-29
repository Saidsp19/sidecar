#ifndef LOGGER_CONFIGURATOR_H	// -*- C++ -*-
#define LOGGER_CONFIGURATOR_H

#include <iosfwd>
#include <set>

#include "Logger/Exceptions.h"
#include "Logger/Log.h"
#include "Logger/Writers.h"

namespace Logger {

/**
   Logger configuration manager class. A configuration is defined by the contents of a config file (or stream),
   where each line specifies a Log object and its setup. The format of a config line is:

   <center><tt>
   NAME [priority P] [[writer W [W_ARGS] formatter F [F_ARGS]] ...]
   </tt></center>

   where \c NAME is the name of the Log object (including lineage); \c P is the priority level the Log object
   will accept messages; \c W is the name of a Writer object to assign to the Log object; \c W_ARGS are any
   optional settings for the Writer constructor; \c F is the name of the Formatter object to use for the Writer;
   and \c F_ARGS are any optional settings for the Formatter constructor.

   The \e priority and \e writer constructs are optional; there may be either or both on the same line, or they
   may appear on separate config lines. There may even be more than one \e writer specification on the same
   line, or each one may appear on a separate config line. In short, all settings are additive and all config
   lines are processed in the order in which they appear in a file or stream.

   The following are valid \c W values for the \e writer keyword:

   <DL>
   <DT>\c cout </DT>
   <DD>Creates a new StreamWriter object, writing to the std::cout stream. The only option available to this
   (and all other) StreamWriter object is

   <tt>[flush]</tt>
   
   which controls whether the stream's flush method is called after each log message write.</DD>

   <DT>\c clog </DT>
   <DD>Same as above except the StreamWriter object uses std::clog for its output stream</DD>
   
   <DT>\c cerr </DT>
   <DD>Same as above except the StreamWriter object uses std::cerr for its output stream</DD>

   <DT>\c file </DT>
   <DD>Creates a new FileWriter object. The arguments to a FileWriter are:

   <tt>PATH [nappend | mode M]</tt>

   where \c PATH specifies a file used to hold log messages (required), and \c M is an optional mode
   specification used for newly-created files (see open(2))

   If \c nappend is found, then FileWriter will overwrite any existing log file; the default behavior is to
   append to the end of an existing file.</DD>

   <DT>\c rolling </DT>
   <DD>Create a new RollingWriter object. The arguments to a RollingWriter are:

   <tt>PATH [nappend | mode M | versions V | size S]</tt>

   where \c PATH, \c M, and \c nappend are the same as for FileWriter; \c V is the number of versions of the log
   file to keep around; \c S is the maximum size of the log file -- if the log file reaches this size, then the
   file is versioned, and then a new log file is created.</DD>

   <DT>\c syslog </DT>
   <DD> Create a new SyslogWriter object which sends all log messages to a syslog daemon (see syslog(3C)). The
   arguments to a SyslogWriter are:

   <tt>IDENT [facility F]</tt>
   
   where \c IDENT is the required identification string given to openlog and found in all log messages generated
   by syslog. \c F is an optional facility specification as defined by the syslog implementation.</DD>

   <DT>\c remote </DT>
   <DD>Create a new RemoteSyslogWriter object which will send all log messages over the network as datagrams to a
   remote syslog server. The arguments to a RemoteSyslogWriter are:

   <tt>HOST [port P]</tt>

   where \c HOST is the required host name or IP address of the remote host to connect to, and \c P is an
   optional port number (default is 514).</DD> </DL>

   The following are valid \c F values for the \e formatter keyword:

   <DL>
   <DT>\c terse </DT>
   <DD>Use a TerseFormatter object for log message formatting.</DD>
   <DT>\c verbose </DT>
   <DD>Use a VerboseFormatter object for log message formatting.</DD>
   <DT>\c pattern </DT>
   <DD> Use a PatternFormatter object for log message formatting. The sole argument to a PatternFormatter is a
   string that specifies how to format a log message.</DD>
   </DL>

   \par Test Cases:
   see ConfiguratorTests.cc
*/
class Configurator
{
public:

    /** Top-level exception class used by all Configurator exceptions.
     */
    template <typename T>
    struct ConfiguratorException : public LoggerException<T>
    {
	/** Constructor.

	    \param routine name of the routine that throwing the exception

	    \param err text of the error
	*/
	ConfiguratorException(const char* routine, const char* err)
	    : LoggerException<T>("Configurator", routine) { *this << err; }

	/** Constructor.

            \param routine name of the routine that throwing the exception

	    \param err text of the error

	    \param log log device being configured at the time of the exception
	*/
	ConfiguratorException(const char* routine, const char* err, const Log& log)
	    : LoggerException<T>("Configurator", routine)
	    { *this << err << log.name(); }
    };

    /** Exception thrown if no name found for Log instance.
     */
    struct MissingLoggerName : public ConfiguratorException<MissingLoggerName> { MissingLoggerName(); };

    /** Exception thrown if no writer type found.
     */
    struct MissingWriterType : public ConfiguratorException<MissingWriterType>
    { MissingWriterType(const Log& log); };

    /** Exception thrown if command is not \c writer or \c priority
     */
    struct InvalidCommand : public ConfiguratorException<InvalidCommand>
    { InvalidCommand(const std::string& cmd); };

    /** Exception thrown if \c priority command does not have a value.
     */
    struct MissingPriorityName : public ConfiguratorException<MissingPriorityName>
    { MissingPriorityName(const Log& log); };

    /** Exception thrown if a closing single- or double-quote is not found in a configuration line.
     */
    struct UnterminatedString : public ConfiguratorException<UnterminatedString>
    { UnterminatedString(const std::string& line); };

    /** Exception thrown if \c writer command does not have a valid type value
     */
    struct InvalidWriterType : public ConfiguratorException<InvalidWriterType>
    { InvalidWriterType(const Log& log, const std::string& type); };

    /** Exception thrown if \c formatter command does not have a type value.
     */
    struct MissingFormatterType : public ConfiguratorException<MissingFormatterType>
    { MissingFormatterType(const Log& log); };

    /** Exception thrown if \c formatter command does not have a valid type value.
     */
    struct InvalidFormatterType : public ConfiguratorException<InvalidFormatterType>
    { InvalidFormatterType(const Log& log, const std::string& type); };

    /** Exception thrown if \c formatter \c pattern does not have a format specifier string.
     */
    struct MissingFormatterPattern : public ConfiguratorException<MissingFormatterPattern>
    { MissingFormatterPattern(const Log& log); };

    /** Exception thrown if StreamWriter definition has an invalid option name.
     */
    struct InvalidStreamFlag : public ConfiguratorException<InvalidStreamFlag>
    { InvalidStreamFlag(const Log& log, const std::string& flag); };

    /** Exception thrown if FileWriter definition does not have a file path value.
     */
    struct MissingFilePath : public ConfiguratorException<MissingFilePath>
    { MissingFilePath(const Log& log); };

    /** Exception thrown if FileWriter definition has an invalid option name.
     */
    struct InvalidFileFlag : public ConfiguratorException<InvalidFileFlag>
    { InvalidFileFlag(const Log& log, const std::string& flag); };

    /** Exception thrown if FileWriter definition has the \c mode option, but does not have a \c mode value.
     */
    struct MissingFileMode : public ConfiguratorException<MissingFileMode>
    { MissingFileMode(const Log& log); };

    /** Exception thrown if RollingWriter definition has the \c versions option, but no \c versions value.
     */
    struct MissingNumVersions : public ConfiguratorException<MissingNumVersions>
    { MissingNumVersions(const Log& log); };

    /** Exception thrown if RollingWriter definition has the \c size option, but no \c size value.
     */
    struct MissingMaxSize : public ConfiguratorException<MissingMaxSize>
    { MissingMaxSize(const Log& log); };

    /** Exception thrown if SyslogWriter definition does not have an ident value.
     */
    struct MissingIdent : public ConfiguratorException<MissingIdent>
    { MissingIdent(const Log& log); };

    /** Exception thrown if SyslogWriter definition has the \c facility option, but no \c facility value.
     */
    struct MissingFacility : public ConfiguratorException<MissingFacility>
    { MissingFacility(const Log& log); };

    /** Exception thrown if SyslogWriter definition has an invalid option name.
     */
    struct InvalidSyslogFlag : public ConfiguratorException<InvalidSyslogFlag>
    { InvalidSyslogFlag(const Log& log, const std::string& flag); };

    /** Exception thrown if RemoteSyslogWriter does not have a host name value.
     */
    struct MissingRemoteHost : public ConfiguratorException<MissingRemoteHost>
    { MissingRemoteHost(const Log& log); };

    /** Exception thrown if RemoteSyslogWriter definition has the \c port option, but no \c port value
     */
    struct MissingRemotePort : public ConfiguratorException<MissingRemotePort>
    { MissingRemotePort(const Log& log); };

    /** Exception thrown if RemoteSyslogWriter definition has an invalid option name.
     */
    struct InvalidRemoteSyslogFlag : public ConfiguratorException<InvalidRemoteSyslogFlag>
    { InvalidRemoteSyslogFlag(const Log& log, const std::string& flag); };

    /** Default constructor. No configuration is loaded.
     */
    Configurator() : managed_() {}

    /** Constructor. Load configuration from a stream

	\param is C++ standard input stream to read from
    */
    Configurator(std::istream& is);

    /** Read configuration settings from a stream.

	\param is C++ standard input stream to read from
    */
    void load(std::istream& is);

protected:

    /** Accessor for the set of Log objects created by Configurator::load

	\return set of managed Log objects
    */
    const std::set<Log*>& managed() const { return managed_; }

private:

    /** Read a priority setting and use to set the priority of a Log object.

        \param is stream to read from

	\param log Log object to manipulate
    */
    void setPriorityLimit(std::istream& is, Log& log) const;

    /** Utility method that reads a quoted string from a C++ input stream. Returns an empty string if EOF on
        stream. Skips over If the first non-space character is not a single- or double-quote character, then
        this returns a sequence of non-space characters found on the stream. All reading is terminted by a
        newline character, but if the reading started with a single- or double-quote character, then an
        exception is raised.

        \param is C++ input stream to read from

        \return C++ string containing data read in
    */
    std::string readQuotedString(std::istream& is) const;

    /** Read a formatter specification, create a Formatter object, and return it

        \param is stream to read from

	\param log Log object to manipulate

	\return new Formatter object
    */
    Formatters::Formatter::Ref makeFormatter(std::istream& is, Log& log) const;

    /** Read a writer specification, create a new Writer object, and add to a Log object.

        \param is stream to read from

	\param log Log object to manipulate
    */
    void addWriter(std::istream& is, Log& log) const;

    /** Create a new StreamWriter object and add to a Log object

        \param is stream to read from

	\param log Log object to manipulate

	\param os C++ output stream to provide StreamWriter
    */
    void addStreamWriter(std::istream& is, Log& log, std::ostream& os) const;

    /** Read in arguments for a FileWriter object, create, and add to a Log object.

        \param is stream to read from

	\param log Log object to manipulate
    */
    void addFileWriter(std::istream& is, Log& log) const;

    /** Read in arguments for a RollingWriter object, create, and add to a Log object.

        \param is stream to read from

	\param log Log object to manipulate
    */
    void addRollingWriter(std::istream& is, Log& log) const;

    /** Set the priority of a Log object.

        \param is stream to read from

	\param log Log object to manipulate
    */
    void addSyslogWriter(std::istream& is, Log& log) const;

    /** Set the priority of a Log object.

        \param is stream to read from

	\param log Log object to manipulate
    */
    void addRemoteSyslogWriter(std::istream& is, Log& log) const;

    std::string currentLine_;	///< Current configuration line being processed
    std::set<Log*> managed_;	///< Collection of Log objects we created

};				// class Configurator
} // end namespace Logger

/** \file
 */

#endif
