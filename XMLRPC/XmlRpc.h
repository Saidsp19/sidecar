#ifndef _XMLRPC_H_
#define _XMLRPC_H_
#if defined(_MSC_VER)
#pragma warning(disable : 4786) // identifier was truncated in debug info
#endif

#ifndef MAKEDEPEND
#include <string>
#endif

#include "XmlRpcClient.h"
#include "XmlRpcException.h"
#include "XmlRpcServer.h"
#include "XmlRpcServerMethod.h"
#include "XmlRpcUtil.h"
#include "XmlRpcValue.h"

namespace XmlRpc {

//! An interface allowing custom handling of error message reporting.
class XmlRpcErrorHandler {
public:
    virtual ~XmlRpcErrorHandler() {}

    //! Returns a pointer to the currently installed error handling object.
    static XmlRpcErrorHandler* getErrorHandler() { return _errorHandler; }

    //! Specifies the error handler.
    static void setErrorHandler(XmlRpcErrorHandler* eh) { _errorHandler = eh; }

    //! Report an error. Custom error handlers should define this method.
    virtual void error(const char* msg) = 0;

protected:
    static XmlRpcErrorHandler* _errorHandler;
};

//! An interface allowing custom handling of informational message reporting.
class XmlRpcLogHandler {
public:
    //! Returns a pointer to the currently installed message reporting object.
    static XmlRpcLogHandler* getLogHandler() { return _logHandler; }

    //! Specifies the message handler.
    static void setLogHandler(XmlRpcLogHandler* lh) { _logHandler = lh; }

    //! Returns the level of verbosity of informational messages. 0 is no output, 5 is very verbose.
    static int getVerbosity() { return _verbosity; }

    //! Specify the level of verbosity of informational messages. 0 is no output, 5 is very verbose.
    static void setVerbosity(int v) { _verbosity = v; }

    virtual ~XmlRpcLogHandler() {}

    //! Output a message. Custom error handlers should define this method.
    virtual void log(int level, const char* msg) = 0;

protected:
    static XmlRpcLogHandler* _logHandler;
    static int _verbosity;
};

//! Returns log message verbosity. This is short for XmlRpcLogHandler::getVerbosity()
int getVerbosity();
//! Sets log message verbosity. This is short for XmlRpcLogHandler::setVerbosity(level)
void setVerbosity(int level);

//! Version identifier
extern const char XMLRPC_VERSION[];

} // namespace XmlRpc

/** \file
 */

#endif // _XMLRPC_H_
