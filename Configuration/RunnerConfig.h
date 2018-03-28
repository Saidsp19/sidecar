#ifndef SIDECAR_CONFIGURATION_RUNNERCONFIG_H // -*- C++ -*-
#define SIDECAR_CONFIGURATION_RUNNERCONFIG_H

#include "QtCore/QList"
#include "QtCore/QString"

#include "QtXml/QDomElement"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Configuration {

class Loader;

/** Simple container class that holds attributes relating to a particular runner in a configuration file.
 */
class RunnerConfig {
public:
    /** Obtain the log device for RunnerConfig objects.

        \return Logger::Log reference
    */
    static Logger::Log& Log();

    struct Init {
        Init() :
            name(""), opts(""), host("localhost"), multicastAddress("237.1.2.100"), scheduler("SCHED_INHERIT"),
            priority("ACE_DEFAULT_THREAD_PRIORITY"), cpuAffinity("0"), initialProcessingState("run")
        {
        }

        QString name;
        QString opts;
        QString host;
        QString multicastAddress;
        QString scheduler;
        QString priority;
        QString cpuAffinity;
        QString initialProcessingState;
        QList<QDomElement> streams;
    };

    /** Constructor for a new RunnerConfig object

        \param configurationName name of the configuration we belong to

        \param configurationPath path of the configuration file we belong to

        \param runnerName the name of the runner process being configured

        \param hostName name of the host that is running the runner process

        \param multicastAddress address to use for all multicast emissions

        \param logsDirectory location where log files will be deposited

        \param streams
    */
    RunnerConfig(const Loader& loader, const Init& init);

    /** Obtain the name of the configuration

        \return configuration name
    */
    const QString& getConfigurationName() const { return configurationName_; }

    /** Obtain the name of the runner in the configuration

        \return runner name
    */
    const QString& getRunnerName() const { return cfg_.name; }

    /** Obtain the name of the host on which the runner will run

        \return host name
    */
    const QString& getHostName() const { return cfg_.host; }

    /** Obtain the multicast address to use for multicast emissions

        \return multicast address in dot notation
    */
    const QString& getMulticastAddress() const { return cfg_.multicastAddress; }

    /** Obtain the service name for the runner. This is a combination of the configuration name, host name, and
        runner name, separated by a ':' character.

        \return service name
    */
    const QString& getServiceName() const { return serviceName_; }

    /** Obtain the requested scheduler for the runner. This can be one of : SCHED_INHERIT, SCHED_OTHER,
        SCHED_FIFO, SCHED_RR

        \return thread flags
    */
    const QString& getScheduler() const { return cfg_.scheduler; }

    /** Obtain the requested priority for the runner.

        \return priority
    */
    const QString& getPriority() const { return cfg_.priority; }

    /** Obtain the requested cpu affinity for the runner.

        \return cpuaffinity
    */
    const QString& getCpuAffinity() const { return cfg_.cpuAffinity; }

    const QString& getInitialProcessingState() const { return cfg_.initialProcessingState; }

    /** Obtain the location of the log file for the runner process

        \return log file path
    */
    const QString& getLogPath() const { return logPath_; }

    /** Obtain the command to execute that will run the runner process on the configured remote host.

        \return command line string
    */
    const QString& getRemoteCommand() const { return remoteCommand_; }

    /** Obtain a list of XML nodes that define the streams in the runner process.

        \return QList of XML nodes
    */
    const QList<QDomElement>& getStreamNodes() const { return cfg_.streams; }

    /** Update the service name due to a name conflict with something else running in the local LAN.

        \param serviceName new name to use for the service
    */
    void setServiceName(const QString& serviceName);

private:
    QString configurationName_;
    Init cfg_;
    QString serviceName_;
    QString logPath_;
    QString remoteCommand_;
};

} // end namespace Configuration
} // end namespace SideCar

#endif
