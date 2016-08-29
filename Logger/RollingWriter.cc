#include <cstdio>		// for rename
#include <unistd.h>		// for lseek

#include "Writers.h"

using namespace Logger;
using namespace Logger::Writers;

void
RollingFile::open()
{
    File::open();
    if (fd() != -1) fileSize_ = ::lseek(fd(), 0, SEEK_END);
}

void
RollingFile::write(const Msg& msg)
   
{
    if (fileSize_ >= maxSize_) rollover();
    File::write(msg);
    fileSize_ += wrote();
}

void
RollingFile::rollover()
{
    close();
    if (numVersions_ == 0) {
	::unlink(path().c_str());
    }
    else {

	// Generate a template for renaming files. Append '.' + sequence number
	// to the path of the log file.
	//
	std::ostringstream oss;
	oss << path() << '.';
	std::ostringstream::pos_type pos = oss.tellp();
	oss << numVersions_;
	std::string from(oss.str());
	std::string to;

	// Rename files so that we lose the oldest version, and younger
	// versions have their version number incremented by one.
	//
	for (int index = numVersions_ - 1; index > 0; --index) {
	    to = from;
	    oss.seekp(pos);
	    oss << index;
	    from = oss.str();
	    ::rename(from.c_str(), to.c_str());
	}

	// Finally, rename our log file to the first versioned name.
	//
	::rename(path().c_str(), from.c_str());
    }

    // Reopen file.
    //
    open();
}

