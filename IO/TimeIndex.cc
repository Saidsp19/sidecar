#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "Logger/Log.h"
#include "Messages/Header.h"
#include "Utils/FilePath.h"

#include "AutoCloseFileDescriptor.h"
#include "TimeIndex.h"

using namespace SideCar::IO;

std::string const TimeIndex::kIndexFileSuffix_("timeIndex");

Logger::Log&
TimeIndex::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.TimeIndex");
    return log_;
}

std::string const&
TimeIndex::GetIndexFileSuffix()
{
    return kIndexFileSuffix_;
}

TimeIndex::TimeIndex(const std::string& path)
    : array_(0), size_(0)
{
    static Logger::ProcLog log("TimeIndex", Log());
    LOGINFO << path << std::endl;

    Utils::FilePath fp(path);
    if (! fp.exists()) {
	Utils::Exception ex("Unable to locate index file ");
	ex << fp;
	log.thrower(ex);
    }

    LOGDEBUG << "loading position file" << std::endl;
    load(fp);
}

TimeIndex::~TimeIndex()
{
    if (array_) ::munmap(array_, size_ * sizeof(Entry));
}

void
TimeIndex::load(const Utils::FilePath& path)
{
    static Logger::ProcLog log("load", Log());
    LOGINFO << path << std::endl;

    AutoCloseFileDescriptor ifd(::open(path.c_str(), O_RDONLY));
    if (! ifd) {
	Utils::Exception ex("Failed to open position file ");
	ex << path << " - " << errno << ' ' << strerror(errno);
	log.thrower(ex);
    }

    struct stat fileStats;
    int rc = ::fstat(ifd, &fileStats);
    if (rc == -1) {
	Utils::Exception ex("Failed fstats() on position file ");
	ex << path << " - " << errno << ' ' << strerror(errno);
	log.thrower(ex);
    }

    size_ = fileStats.st_size / sizeof(Entry);
    array_ = (Entry*)::mmap(0, fileStats.st_size, PROT_READ, MAP_SHARED,
                            ifd, 0);
    if (array_ == (void*)(-1)) {
	Utils::Exception ex("Failed mmap() on position file ");
	ex << path << " - " << errno << ' ' << strerror(errno);
	LOGERROR << ex.err() << std::endl;
	size_ = 0;
	array_ = 0;
    }
}

off_t
TimeIndex::findOnOrBefore(uint32_t when) const
{
    static Logger::ProcLog log("find", Log());
    LOGINFO << when << std::endl;
    off_t offset = 0;
    if (size_) {
	Entry tmp(when);
	const_iterator pos(std::lower_bound(begin(), end(), tmp));
	if (pos == end()) {
	    offset = off_t(-1);
	}
	else {
	    if (pos != begin() && pos->when_ > when) --pos;
	    offset = pos->position_;
	}
    }

    LOGDEBUG << offset << std::endl;
    return offset;
}
