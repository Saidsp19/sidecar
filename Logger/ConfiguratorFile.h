#ifndef LOGGER_CONFIGURATORFILE_H // -*- C++ -*-
#define LOGGER_CONFIGURATORFILE_H

#include <iosfwd>
#include <sys/time.h> // for time_t

#include "Logger/Configurator.h"
#include "Threading/Threading.h"

namespace Logger {

class Monitor;

/** Derivation of Configurator that takes configuration settings from a file. Supports configuration reloading
    and detection of changed configuration files. For information on the configuration file format, see
    Configurator.
*/
class ConfiguratorFile : public Configurator {
public:
    /** Exception thrown if unable to open a configuration file.
     */
    struct CannotOpenConfigFile : public ConfiguratorException<CannotOpenConfigFile> {
        CannotOpenConfigFile(const std::string& path);
    };

    /** Exception thrown if unable to access (see) the configuration file.
     */
    struct CannotAccessConfigFile : public ConfiguratorException<CannotAccessConfigFile> {
        CannotAccessConfigFile(const std::string& path);
    };

    /** Constructor. Use configuration settings found in a file.

        \param path location of file to use
    */
    ConfiguratorFile(const std::string& path);

    /** Destructor. Stop monitor thread if active.
     */
    ~ConfiguratorFile();

    /** Reread the configuration settings and update all managed Log objects.
     */
    void reload();

    /** Obtain the system's modification time for the configuration file.

        \return time when file was last modified
    */
    time_t getModificationTime() const;

    /** Check if configuration file is newer than when it was last read in.

        \return true if newer
    */
    bool isStale() const { return getModificationTime() > lastModification_; }

    /** See if configuration file has changed, and if so reload its contents.
     */
    void check()
    {
        if (isStale()) reload();
    }

    /** Spawn a new thread that will periodically check to see if the configuration file has changed, and if so
        it will reload it. NOTE: this routine starts a new thread.

        \param period number of seconds to sleep between check calls

        \return result of pthread_create call (0 is OK)
    */
    void startMonitor(double period);

    /** Stop a previously-started monitor.
     */
    void stopMonitor();

private:
    std::string path_;        ///< Location of configuration file
    time_t lastModification_; ///< Timestamp when file was last modified

    /** Support class that monitors a configuration file for changes. If a change is detected, it causes the
        ConfigurationFile instance to reload.
    */
    struct Monitor : public Threading::Thread {
        /** Constructor.

            \param config ConfiguratorFile instance to work with

            \param sleep number of seconds between file change checks
        */
        Monitor(ConfiguratorFile& config, double sleep);

        /** Implementation of Thread::run interface. Contains a loop that checks for file changes.
         */
        void run();

        /** Tell the monitor thread to stop. Returns immediately
         */
        void stop();

        /** Stop the monitor. Returns when the thread has finished processing and has exited.
         */
        void die();

        ConfiguratorFile& config_; ///< Instance whose file we monitor.
        double sleep_;             ///< Amount of time to sleep between checks.
        Threading::Condition::Ref stopRunningCondition_;
        bool stopRunning_; ///< Variable guarded by above condition.
    };

    Monitor* monitor_; ///< Monitor object, if running
};

} // end namespace Logger

/** \file
 */

#endif
