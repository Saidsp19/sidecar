#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "Logger/Log.h"
#include "Messages/Header.h"
#include "Utils/FilePath.h"

#include "AutoCloseFileDescriptor.h"
#include "RecordIndex.h"

using namespace SideCar::IO;

std::string const RecordIndex::kIndexFileSuffix_("recordIndex");

Logger::Log&
RecordIndex::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.RecordIndex");
    return log_;
}

std::string const&
RecordIndex::GetIndexFileSuffix()
{
    return kIndexFileSuffix_;
}

RecordIndex::RecordIndex(const std::string& path)
    : array_(0), size_(0)
{
    static Logger::ProcLog log("RecordIndex", Log());
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

RecordIndex::~RecordIndex()
{
    if (array_) ::munmap(array_, size_ * sizeof(off_t));
}

void
RecordIndex::load(const Utils::FilePath& path)
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

    size_ = fileStats.st_size / sizeof(off_t);
    array_ = (off_t*)::mmap(0, fileStats.st_size, PROT_READ, MAP_SHARED,
                            ifd, 0);
    if (array_ == (void*)(-1)) {
	Utils::Exception ex("Failed mmap() on position file ");
	ex << path << " - " << errno << ' ' << strerror(errno);
	LOGERROR << ex.err() << std::endl;

	size_ = 0;
	array_ = 0;
    }
}
