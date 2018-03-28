// -*- C++ -*-

#include <fcntl.h>
#include <iostream>
#include <unistd.h> // for ::close

#include "Writers.h"

using namespace Logger;
using namespace Logger::Writers;

File::File(const Formatters::Formatter::Ref& formatter, const std::string& path, bool append, mode_t mode,
           bool flushAfterUse) :
    Writer(formatter, flushAfterUse),
    buffer_(""), path_(path), fd_(-1), openFlags_(O_CREAT | O_APPEND | O_WRONLY), openMode_(mode)
{
    if (!append) openFlags_ |= O_TRUNC;
#ifdef solaris
    openFlags_ |= O_LARGEFILE;
#endif
#ifndef darwin
    if (flushAfterUse) openFlags_ |= O_DSYNC;
#endif
}

void
File::open()
{
    close();
    fd_ = ::open(path_.c_str(), openFlags_, openMode_);
    if (fd_ == -1) { std::cerr << "*** FileObj: failed to open file " << path_ << '\n'; }
}

void
File::close()
{
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

void
File::flush()
{
#ifdef darwin
    ::fsync(fd_);
#endif
}

void
File::write(const Msg& msg)
{
    if (fd_ == -1) open();
    if (fd_ == -1) return;
    buffer_.str("");
    format(buffer_, msg);

    std::string s = buffer_.str();
    if (::write(fd_, &s[0], s.size()) == -1) {
        close();
    } else {
        if (getFlushAfterUse()) flush();
    }
}
