#ifndef LOGGER_WRITERS_H	// -*- C++ -*-
#define LOGGER_WRITERS_H

#include <sys/types.h>		// for type mode_t

#ifdef _WIN32

using mode_t = int;
#include <basetsd.h>
using ssize_t = SSIZE_T;
#define NOGDI
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h> // for sockaddr_in

#else

#include <netinet/in.h>
#include <arpa/inet.h>

#endif

#include <sstream>

#include "boost/shared_ptr.hpp"

#include "Logger/Formatters.h"
#include "Logger/Priority.h"

namespace Logger {

struct Msg;

/** Namespace for Log device writers.
 */
namespace Writers
{

/** Abstract class that defines the interface for all objects that write log messages.
 */
class Writer
{
public:

    using Ref = boost::shared_ptr<Writer>;

    /** Destructor. Delete registered Formatter object and remove from set of created writers.
     */
    virtual ~Writer() {}

    /** Open connection to storage device. Subclass must define.
     */
    virtual void open() {};

    /** Close connection to storage device. Subclass must define.
     */
    virtual void close() {};

    /** Close and then reopen connection to storage device.
     */
    virtual void reopen() { close(); open(); }

    /** Flush any pending messages to the storage device.
     */
    virtual void flush() {};

    /** Write out a Msg instance to the storage device.

        \param msg Msg instance to format and write out
    */
    virtual void write(const Msg& msg) = 0;

    bool operator <(const Writer& rhs) const { return this < &rhs; }
    
    void setFlushAfterUse(bool enabled) { flushAfterUse_ = enabled; }

    bool getFlushAfterUse() const { return flushAfterUse_; }

protected:

    /** Constructor. Only derived classes can see.

        \param formatter Formatter instances used to generate a std::string value from a Msg instance. Acquires
        ownership of the Formatter instace, and will dispose of it in the destructor.
    */
    Writer(Formatters::Formatter::Ref formatter, bool flushAfterUse)
        : formatter_(formatter), flushAfterUse_(flushAfterUse) {}

    void format(std::ostream& os, const Msg& msg) { formatter_->format(os, msg); }

private:
    Formatters::Formatter::Ref formatter_;
    bool flushAfterUse_;
};

class File : public Writer
{
public:
    using Ref = boost::shared_ptr<File>;
    static Ref Make(const Formatters::Formatter::Ref& formatter, const std::string& path, bool append = true,
                    mode_t mode = 00644, bool flushAfterUse = false)
        {Ref ref(new File(formatter, path, append, mode, flushAfterUse)); return ref;}
    
    ~File() { close(); }

    void open() override;
    void close() override;
    void flush() override;
    void write(const Msg& msg) override;

    const std::string& path() const { return path_; }

protected:
    File(const Formatters::Formatter::Ref& formatter, const std::string& path, bool append, mode_t mode,
         bool flushAfterUse);

    size_t wrote() const { return buffer_.str().size(); }
    int fd() const { return fd_; }

private:
    std::ostringstream buffer_;
    const std::string path_;
    int fd_;
    int openFlags_;
    mode_t openMode_;
};				// class File

class RollingFile : public File
{
public:
    using Ref = boost::shared_ptr<RollingFile>;
    static Ref Make(const Formatters::Formatter::Ref& formatter, const std::string& path, ssize_t maxSize,
                    int numVersions, bool append = true, mode_t mode = 00644, bool flushAfterUse = false)
        {
            Ref ref(new RollingFile(formatter, path, maxSize, numVersions, append, mode, flushAfterUse));
            return ref;
        }
    
    void open() override;
    void write(const Msg& msg) override;

private:
    RollingFile(const Formatters::Formatter::Ref& formatter, const std::string& path, ssize_t maxSize,
                int numVersions, bool append, mode_t mode, bool flushAfterUse)
	: File(formatter, path, append, mode, flushAfterUse), maxSize_(maxSize), fileSize_(0),
          numVersions_(numVersions) {}

    void rollover();

    size_t maxSize_;
    size_t fileSize_;
    int numVersions_;
};				// class RollingFile

class Stream : public Writer
{
public:
    using Ref = boost::shared_ptr<Stream>;
    static Ref Make(const Formatters::Formatter::Ref& formatter, std::ostream& os, bool flushAfterUse = false)
        {Ref ref(new Stream(formatter, os, flushAfterUse)); return ref;}

    void flush() override;
    void write(const Msg& msg) override;

private:
    Stream(const Formatters::Formatter::Ref& formatter, std::ostream& os, bool flushAfterUse)
	: Writer(formatter, flushAfterUse), os_(os) {}

    std::ostream& os_;
};				// class Stream

class Syslog : public Writer
{
public:
    using Ref = boost::shared_ptr<Syslog>;
    static Ref Make(const Formatters::Formatter::Ref& formatter, const std::string& ident, int facility,
                    bool flushAfterUse = false)
        {
            Ref ref(new Syslog(formatter, ident, facility, flushAfterUse));
            return ref;
        }

    ~Syslog() { close(); }

    static int ConvertPriority(Priority::Level level) throw(std::invalid_argument);

    void open() override;
    void close() override;
    void write(const Msg& msg) throw(std::invalid_argument) override;

private:
    Syslog(const Formatters::Formatter::Ref& formatter, const std::string& ident, int facility,
           bool flushAfterUse)
	: Writer(formatter, flushAfterUse), buffer_(""), ident_(ident), facility_(facility) {}

    std::ostringstream buffer_;
    const std::string ident_;
    int facility_;
};				// class Syslog

class RemoteSyslog : public Writer
{
public:
    using Ref = boost::shared_ptr<RemoteSyslog>;

    static Ref Make(const Formatters::Formatter::Ref& formatter, const std::string& host, int port = 514,
                    bool flushAfterUse = false)
        {
            Ref ref(new RemoteSyslog(formatter, host, port, flushAfterUse));
            return ref;
        }
    
    virtual void open();
    virtual void close();
    virtual void write(const Msg& msg) throw(std::invalid_argument);

private:
    RemoteSyslog(const Formatters::Formatter::Ref& formatter, const std::string& host, int port,
                 bool flushAfterUse)
	: Writer(formatter, flushAfterUse), host_(host), port_(port), socket_(-1), buffer_(""), addr_() {}

    std::string host_;
    int port_;
    int socket_;
    std::ostringstream buffer_;
    sockaddr_in addr_;
};				// class RemoteSyslog

}				// namespace Writers
}				// namespace Logger

/** \file
 */

#endif
