#include "LogStreamBuf.h"
#include "Log.h"
#include "Priority.h"

using namespace Logger;

LogStreamBuf::~LogStreamBuf()
{
    sync();
}

int
Logger::LogStreamBuf::sync()
{
    int rc = std::stringbuf::sync();
    if (log_) log_->post(level_, str());
    str("");
    return rc;
}

void
LogStreamBuf::setInfo(Log* log, Priority::Level level)
{
    log_ = log;
    level_ = level;
}
