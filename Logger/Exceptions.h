#ifndef LOGGER_EXCEPTIONS_H // -*- C++ -*-
#define LOGGER_EXCEPTIONS_H

#include "Utils/Exception.h"
#include <string>

namespace Logger {

/** Top-level exception thrown by the Logger classes.
 */
template <typename T>
struct LoggerException : public Utils::Exception, public Utils::ExceptionInserter<T> {
    LoggerException(const char* module, const char* routine) : Utils::Exception("Logger::")
    {
        *this << module << "::" << routine << ": ";
    }
};

/** Exception thrown for when Log::Find is given an invalid name
 */
struct InvalidLoggerName : public LoggerException<InvalidLoggerName> {
    InvalidLoggerName(const std::string& name) : LoggerException<InvalidLoggerName>("Logger", "Find")
    {
        *this << "invalid Logger name - '" << name << "'";
    }
};

} // end namespace Logger

/** \file
 */

#endif
