#include <syslog.h>

#include "Msg.h"
#include "Writers.h"

using namespace Logger;
using namespace Logger::Writers;

int
Syslog::ConvertPriority(Priority::Level level)
{
    switch (level) {
    case Priority::kFatal: return LOG_CRIT;
    case Priority::kError: return LOG_ERR;
    case Priority::kWarning: return LOG_WARNING;
    case Priority::kInfo: return LOG_INFO;
    case Priority::kTraceIn: return LOG_DEBUG;
    case Priority::kTraceOut: return LOG_DEBUG;
    case Priority::kDebug1: return LOG_DEBUG;
    case Priority::kDebug2: return LOG_DEBUG;
    case Priority::kDebug3: return LOG_DEBUG;
    default: throw std::invalid_argument("invalid Priority::Level"); break;
    }
}

void
Syslog::open()
{
    ::openlog(ident_.c_str(), LOG_PID | LOG_CONS, facility_);
}

void
Syslog::close()
{
    ::closelog();
}

void
Syslog::write(const Msg& msg)
{
    buffer_.str("");
    format(buffer_, msg);
    ::syslog(ConvertPriority(msg.level_), "%s", buffer_.str().c_str());
}
