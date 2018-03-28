#ifndef LOGGER_MSG_H // -*- C++ -*-
#define LOGGER_MSG_H

#ifndef _WIN32
#include <sys/time.h> // for timeval
#endif

#include "Logger/Priority.h"

namespace Logger {

/** Collection of data elements that make up one log message. Contains the following attributes:

    - \c when_ - timestamp of when the log message was created
    - \c channel_ - full-name of the Log instance that posted the message
    - \c message_ - text of the log message
    - \c level_ - priority level of the log message.
*/
struct Msg {
    /** Constructor.

        \param channel name of the Log instance that created the message

        \param message text of the log message

        \param level severity level of the message
    */
    Msg(const std::string& channel, const std::string& message, Priority::Level level) :
        when_(), channel_(channel), message_(message), level_(level)
    {
    }

    ::timeval when_;        ///< Time when Msg instance was created
    std::string channel_;   ///< Name of the Log instance that created us
    std::string message_;   ///< Text of the log message
    Priority::Level level_; ///< Severity level
};

} // namespace Logger

/** \file
 */

#endif
