#include <sys/stat.h>		// for struct stat

#include "Logger/Log.h"
#include "Threading/Threading.h"

#include "FileWatcher.h"

using namespace Utils;

struct FileWatcher::Monitor : public Threading::Thread
{
    Monitor(FileWatcher& config, double sleep)
	: Threading::Thread(), config_(config), sleep_(sleep),
          stopRunningCondition_(Threading::Condition::Make()), stopRunning_(false)
	{}

    void run()
	{
	    Logger::ProcLog log("Monitor::run", Log());
	    LOGINFO << std::endl;
	    Threading::Locker lock(stopRunningCondition_);
	    while (! stopRunning_) {
		LOGDEBUG << "checking for stale file" << std::endl;
		if (config_.isStale()) {
		    LOGWARNING << "file is stale - reloading" << std::endl;
		    config_.reload();
		}
		stopRunningCondition_->timedWaitForSignal(sleep_);
	    }
	}

    void stop()
	{
	    Logger::ProcLog log("Monitor::stop", Log());
	    LOGINFO << std::endl;
	    Threading::Locker lock(stopRunningCondition_);
	    stopRunning_ = true;
	    stopRunningCondition_->signal();
	}

    void die()
	{
	    Logger::ProcLog log("Monitor::die", Log());
	    LOGINFO << std::endl;
	    stop();
	    join();
	}

    FileWatcher& config_;
    double sleep_;
    Threading::Condition::Ref stopRunningCondition_;
    bool stopRunning_;
};

Logger::Log&
FileWatcher::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.FileWatcher");
    return log_;
}

FileWatcher::FileWatcher(double period)
    : period_(period), path_(""), lastModification_(0), monitor_(0)
{
    if (period_ < 0.1) {
	period_ = 0.1;
    }
}

FileWatcher::~FileWatcher()
{
    stopMonitor();
}

time_t
FileWatcher::getModificationTime() const
{
    Logger::ProcLog log("getModificationTime", Log());
    struct stat st;
    int rc = ::stat(path_.c_str(), &st);
    if (rc == -1) st.st_mtime = time_t(-1);
    LOGDEBUG << path_ << ' ' << st.st_mtime << std::endl;
    return st.st_mtime;
}

void
FileWatcher::startMonitor()
{
    Logger::ProcLog log("startMonitor", Log());
    LOGINFO << std::endl;
    monitor_ = new Monitor(*this, period_);
    monitor_->start();
}

void
FileWatcher::stopMonitor()
{
    Logger::ProcLog log("stopMonitor", Log());
    LOGINFO << std::endl;
    if (monitor_) {
	LOGWARNING << std::endl;
	monitor_->die();
	delete monitor_;
	monitor_ = 0;
    }
}

bool
FileWatcher::setFilePath(const std::string& path)
{
    Logger::ProcLog log("setFilePath", Log());
    LOGINFO << path << std::endl;

    stop();
    path_ = path;
    if (path_.size()) {
	if (! reload()) {
	    LOGERROR << "failed to load config file " << path << std::endl;
	    return false;
	}
	start();
    }

    return true;
}

bool
FileWatcher::reload()
{
    Logger::ProcLog log("reload", Log());

    time_t lm = getModificationTime();
    LOGDEBUG << path_ << ' ' << lm << std::endl;

    if (lm == time_t(-1)) {
	LOGERROR << "failed to locate configuration file " << path_ << std::endl;
	return false;
    }

    if (! loadFile(path_)) {
	LOGERROR << "failed to reload configuration file " << path_ << std::endl;
	return false;
    }

    lastModification_ = lm;

    return true;
}
