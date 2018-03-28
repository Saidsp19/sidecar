#include <algorithm>
#include <iostream>

#include "Configurator.h"

using namespace Logger;

Configurator::MissingLoggerName::MissingLoggerName() :
    ConfiguratorException<MissingLoggerName>("load", "failed to read Logger name")
{
    ;
}

Configurator::InvalidCommand::InvalidCommand(const std::string& cmd) :
    ConfiguratorException<InvalidCommand>("load", "invalid configuration command - ")
{
    *this << cmd;
}

Configurator::MissingWriterType::MissingWriterType(const Log& log) :
    ConfiguratorException<MissingWriterType>("addWriter", "failed to read Writer kind for log ", log)
{
    ;
}

Configurator::InvalidWriterType::InvalidWriterType(const Log& log, const std::string& type)

    :
    ConfiguratorException<InvalidWriterType>("addWriter", "invalid LogWriter type for log ", log)
{
    *this << " - " << type;
}

Configurator::MissingPriorityName::MissingPriorityName(const Log& log)

    :
    ConfiguratorException<MissingPriorityName>("setPriorityLimit", "failed to read priority value for log ", log)
{
    ;
}

Configurator::UnterminatedString::UnterminatedString(const std::string& line)

    :
    ConfiguratorException<UnterminatedString>("readQuotedString", "missing string terminator in line - ")
{
    *this << line;
}

Configurator::MissingFormatterType::MissingFormatterType(const Log& log)

    :
    ConfiguratorException<MissingFormatterType>("makeFormatter", "failed to read Formatter kind for log ", log)
{
    ;
}

Configurator::InvalidFormatterType::InvalidFormatterType(const Log& log, const std::string& type) :
    ConfiguratorException<InvalidFormatterType>("makeFormatter", "invalid Formatter type for log ", log)
{
    *this << " - " << type;
}

Configurator::MissingFormatterPattern::MissingFormatterPattern(const Log& log) :
    ConfiguratorException<MissingFormatterPattern>("makeFormatter", "failed to read formatting pattern for log ", log)
{
    ;
}

Configurator::InvalidStreamFlag::InvalidStreamFlag(const Log& log, const std::string& flag) :
    ConfiguratorException<InvalidStreamFlag>("makeStreamWriter", "invalid StreamWriter flag found for log ", log)
{
    *this << " - " << flag;
}

Configurator::MissingFilePath::MissingFilePath(const Log& log) :
    ConfiguratorException<MissingFilePath>("makeFileWriter", "failed to read path of log file for log ", log)
{
    ;
}

Configurator::MissingFileMode::MissingFileMode(const Log& log) :
    ConfiguratorException<MissingFileMode>("makeFileWriter", "failed to read file mode for log ", log)
{
    ;
}

Configurator::InvalidFileFlag::InvalidFileFlag(const Log& log, const std::string& flag) :
    ConfiguratorException<InvalidFileFlag>("makeFileWriter", "invalid file flag found for log ", log)
{
    *this << " - " << flag;
}

Configurator::MissingNumVersions::MissingNumVersions(const Log& log) :
    ConfiguratorException<MissingNumVersions>("makeRollingWriter", "failed to read version count for log ", log)
{
    ;
}

Configurator::MissingMaxSize::MissingMaxSize(const Log& log) :
    ConfiguratorException<MissingMaxSize>("makeRollingWriter", "failed to read max file size for log ", log)
{
    ;
}

Configurator::MissingIdent::MissingIdent(const Log& log) :
    ConfiguratorException<MissingIdent>("makeSyslogWriter", "failed to read syslog ident tag for log ", log)
{
    ;
}

Configurator::MissingFacility::MissingFacility(const Log& log) :
    ConfiguratorException<MissingFacility>("makeSyslogWriter", "failed to read syslog facility for log ", log)
{
    ;
}

Configurator::InvalidSyslogFlag::InvalidSyslogFlag(const Log& log, const std::string& flag) :
    ConfiguratorException<InvalidSyslogFlag>("makeSyslogWriter", "invalid syslog flag for log ", log)
{
    *this << " - " << flag;
}

Configurator::MissingRemoteHost::MissingRemoteHost(const Log& log) :
    ConfiguratorException<MissingRemoteHost>("makeRemoteSyslogWriter", "failed to host name for log ", log)
{
    ;
}

Configurator::MissingRemotePort::MissingRemotePort(const Log& log) :
    ConfiguratorException<MissingRemotePort>("makeRemoteSyslogWriter", "failed to host port for log ", log)
{
    ;
}

Configurator::InvalidRemoteSyslogFlag::InvalidRemoteSyslogFlag(const Log& log, const std::string& flag) :
    ConfiguratorException<InvalidRemoteSyslogFlag>("makeRemoteSyslogWriter", "invalid remote syslog flag for log ", log)
{
    *this << " - " << flag;
}

Configurator::Configurator(std::istream& is) : managed_()
{
    load(is);
}

void
Configurator::load(std::istream& is)
{
    managed_.clear();
    while (std::getline(is, currentLine_)) {
        if (currentLine_.size() == 0 || currentLine_[0] == '#') continue;
        std::istringstream iss(currentLine_);
        std::string logName;
        if (!(iss >> logName)) throw MissingLoggerName();

        // Fetch/create the log object. Remember the Log object as one that we are working with.
        //
        Log& log(Log::Find(logName.c_str()));
        managed_.insert(&log);
        std::string what;
        while ((iss >> what)) {
            if (what == "priority")
                setPriorityLimit(iss, log);
            else if (what == "propagate")
                log.setPropagate(true);
            else if (what == "writer")
                addWriter(iss, log);
            else {
                InvalidCommand ex(what);
                throw ex;
            }
        }
    }
}

void
Configurator::setPriorityLimit(std::istream& is, Log& log) const
{
    std::string name;
    if (!(is >> name)) throw MissingPriorityName(log);
    log.setPriorityLimit(Priority::Find(name));
}

std::string
Configurator::readQuotedString(std::istream& is) const
{
    char c, delim;
    std::string s("");

    // Skip any leading whitespace
    //
    while (is.get(delim) && isspace(delim))
        ;

    if (!is) return s;

    // If not a single or double quote, keep reading until we reach a space or end of line.
    //
    if (delim != '"' && delim != '\'') {
        s += delim;
        delim = ' ';
    }

    while (is.get(c) && c != '\n' && c != delim) s += c;

    // If we had a non-space delimiter, make sure we read it in.
    //
    if (delim != ' ' && c != delim) throw UnterminatedString(currentLine_);

    return s;
}

Formatters::Formatter::Ref
Configurator::makeFormatter(std::istream& is, Log& log) const
{
    std::string kind;
    if (!(is >> kind)) throw MissingFormatterType(log);
    if (kind == "formatter") {
        return makeFormatter(is, log);
    } else if (kind == "terse") {
        return Formatters::Terse::Make();
    } else if (kind == "verbose") {
        return Formatters::Verbose::Make();
    } else if (kind == "pattern") {
        std::string pat(readQuotedString(is));
        if (!is || pat.size() == 0) throw MissingFormatterPattern(log);
        return Formatters::Pattern::Make(pat);
    } else {
        throw InvalidFormatterType(log, kind);
    }
}

void
Configurator::addWriter(std::istream& is, Log& log) const
{
    std::string writerType;
    if (!(is >> writerType)) throw MissingWriterType(log);

    if (writerType == "cout") {
        addStreamWriter(is, log, std::cout);
    } else if (writerType == "clog") {
        addStreamWriter(is, log, std::clog);
    } else if (writerType == "cerr") {
        addStreamWriter(is, log, std::cerr);
    } else if (writerType == "file") {
        addFileWriter(is, log);
    } else if (writerType == "rolling") {
        addRollingWriter(is, log);
    } else if (writerType == "syslog") {
        addSyslogWriter(is, log);
    } else if (writerType == "remote") {
        addRemoteSyslogWriter(is, log);
    } else {
        throw InvalidWriterType(log, writerType);
    }
}

void
Configurator::addStreamWriter(std::istream& is, Log& log, std::ostream& os) const
{
    bool flushAfterUse = false;
    std::string flags;
    while ((is >> flags)) {
        if (flags == "flush") {
            flushAfterUse = true;
        } else if (flags == "nflush") {
            flushAfterUse = false;
        } else if (flags == "formatter") {
            break;
        } else {
            throw InvalidStreamFlag(log, flags);
        }
    }

    log.addWriter(Writers::Stream::Make(makeFormatter(is, log), os, flushAfterUse));
}

void
Configurator::addFileWriter(std::istream& is, Log& log) const
{
    std::string path(readQuotedString(is));
    if (!is || path.size() == 0) throw MissingFilePath(log);

    bool flushAfterUse = false;
    bool append = true;
    mode_t mode = 00644;
    std::string flags;
    while ((is >> flags)) {
        if (flags == "nappend") {
            append = false;
        } else if (flags == "flush") {
            flushAfterUse = true;
        } else if (flags == "nflush") {
            flushAfterUse = false;
        } else if (flags == "mode") {
            if (!(is >> mode)) throw MissingFileMode(log);
        } else if (flags == "formatter") {
            break;
        } else {
            throw InvalidFileFlag(log, flags);
        }
    }

    log.addWriter(Writers::File::Make(makeFormatter(is, log), path, append, mode, flushAfterUse));
}

void
Configurator::addRollingWriter(std::istream& is, Log& log) const
{
    std::string path(readQuotedString(is));
    if (!is || path.size() == 0) throw MissingFilePath(log);

    bool append = true;
    mode_t mode = 00644;
    size_t maxSize = 20 * 1024 * 1024;
    int numVersions = 7;
    std::string flags;
    while ((is >> flags)) {
        if (flags == "nappend") {
            append = false;
        } else if (flags == "mode") {
            if (!(is >> mode)) throw MissingFileMode(log);
        } else if (flags == "versions") {
            if (!(is >> numVersions)) throw MissingNumVersions(log);
        } else if (flags == "size") {
            if (!(is >> maxSize)) throw MissingMaxSize(log);
            maxSize *= 1024 * 1024;
        } else if (flags == "formatter") {
            break;
        } else {
            throw InvalidFileFlag(log, flags);
        }
    }

    log.addWriter(Writers::RollingFile::Make(makeFormatter(is, log), path, maxSize, numVersions, append, mode));
}

void
Configurator::addSyslogWriter(std::istream& is, Log& log) const
{
    std::string ident;
    if (!(is >> ident)) throw MissingIdent(log);

    int facility = 0;
    std::string flags;
    while ((is >> flags)) {
        if (flags == "facility") {
            if (!(is >> facility)) throw MissingFacility(log);
        } else if (flags == "formatter") {
            break;
        } else {
            throw InvalidSyslogFlag(log, flags);
        }
    }

    log.addWriter(Writers::Syslog::Make(makeFormatter(is, log), ident, facility));
}

void
Configurator::addRemoteSyslogWriter(std::istream& is, Log& log) const
{
    std::string host;
    if (!(is >> host)) throw MissingRemoteHost(log);

    int port = 514;
    std::string flags;
    while ((is >> flags)) {
        if (flags == "port") {
            if (!(is >> port)) throw MissingRemotePort(log);
        } else if (flags == "formatter") {
            break;
        } else {
            throw InvalidRemoteSyslogFlag(log, flags);
        }
    }

    log.addWriter(Writers::RemoteSyslog::Make(makeFormatter(is, log), host, port));
}
