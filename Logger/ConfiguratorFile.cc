#include <fstream>
#include <iostream>
#include <sys/stat.h> // for struct stat

#include "ConfiguratorFile.h"

using namespace Logger;

ConfiguratorFile::CannotOpenConfigFile::CannotOpenConfigFile(const std::string& path) :
    ConfiguratorException<CannotOpenConfigFile>("load", "failed to open config file - ")
{
    *this << path;
}

ConfiguratorFile::CannotAccessConfigFile::CannotAccessConfigFile(const std::string& path) :
    ConfiguratorException<CannotAccessConfigFile>("load", "failed to access config file - ")
{
    *this << path;
}

ConfiguratorFile::ConfiguratorFile(const std::string& path) :
    Configurator(), path_(path), lastModification_(0), monitor_(0)
{
    reload();
}

ConfiguratorFile::~ConfiguratorFile()
{
    stopMonitor();
}

/** Functor that causes a Log object to remove all of its registered Writer objects.
 */
struct PurgeLog {
    /** Functor method that invokes Log::removeAllWriters() for a Log device.

        \param log Log device to work on.
    */
    void operator()(Log* log) { log->removeAllWriters(); }
};

void
ConfiguratorFile::reload()
{
    // Fetch the current mod time for the config file. We will adopt this value when we have finished loading.
    //
    time_t lm = getModificationTime();
    std::ifstream fs(path_.c_str());
    if (!fs) throw CannotOpenConfigFile(path_);

    // Have all Log objects we created last time dump their Writer objects. Thus configuration changes will
    // affect how Log objects write out log messages.
    //
    std::for_each(managed().begin(), managed().end(), PurgeLog());
    load(fs);

    // Finally, update the modification time. This will affect the results of future isStale() method calls.
    //
    lastModification_ = lm;
}

time_t
ConfiguratorFile::getModificationTime() const
{
    struct stat st;
    int rc = ::stat(path_.c_str(), &st);
    if (rc == -1) throw CannotAccessConfigFile(path_);
    return st.st_mtime;
}

ConfiguratorFile::Monitor::Monitor(ConfiguratorFile& config, double sleep) :
    Threading::Thread(), config_(config), sleep_(sleep), stopRunningCondition_(Threading::Condition::Make()),
    stopRunning_(false)
{
    // This will spawn a new thread that runs the Monitor::run method.
    //
    start();
}

void
ConfiguratorFile::Monitor::run()
{
    // Loop forever until application quits or we've been asked to stop.
    //
    Threading::Locker lock(stopRunningCondition_);
    while (!stopRunning_) {
        if (config_.isStale()) config_.reload();
        stopRunningCondition_->timedWaitForSignal(sleep_);
    }
}

void
ConfiguratorFile::Monitor::stop()
{
    // Thread-safely change the Monitor run flag, and signal the running thread so that it will check the updated
    // value.
    //
    Threading::Locker lock(stopRunningCondition_);
    stopRunning_ = true;
    stopRunningCondition_->signal();
}

void
ConfiguratorFile::Monitor::die()
{
    // Stop the Monitor thread. Returns after signaling the thread.
    //
    stop();

    // Now wait until the Monitor thread has exited.
    //
    join();
}

void
ConfiguratorFile::startMonitor(double period)
{
    monitor_ = new Monitor(*this, period);
}

void
ConfiguratorFile::stopMonitor()
{
    if (monitor_) {
        // Signal the thread to stop, and wait for it to do so.
        //
        monitor_->die();

        // This is now safe, since the thread is no longer running.
        //
        delete monitor_;
        monitor_ = 0;
    }
}
