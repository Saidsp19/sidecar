#include "QtCore/QRegExp"
#include "QtCore/QStringList"

#include "GUI/LogUtils.h"
#include "Utils/FilePath.h"

#include "Loader.h"
#include "RunnerConfig.h"

using namespace SideCar::Configuration;

Logger::Log&
RunnerConfig::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Configuration.RunnerConfig");
    return log_;
}

RunnerConfig::RunnerConfig(const Loader& loader, const Init& cfg) :
    configurationName_(loader.getConfigurationName()), cfg_(cfg), serviceName_(), logPath_(), remoteCommand_()
{
    Logger::ProcLog log("RunnerConfig", Log());
    LOGINFO << "runnerName: " << cfg_.name << " hostName: " << cfg_.host
            << " multicastAddress: " << cfg_.multicastAddress << " scheduler: " << cfg_.scheduler
            << " priority: " << cfg_.priority << " cpuAffinity: " << cfg_.cpuAffinity << std::endl;

    // Create a service name from the configuration, host, and runner name.
    //
    serviceName_ = QString("%1:%2:%3").arg(configurationName_).arg(cfg_.host).arg(cfg_.name);
    LOGDEBUG << "serviceName: " << serviceName_ << std::endl;

    // Convert any '/' in a name into '_' so we can use it as part of the log.
    //
    QString tmp(cfg_.name);
    tmp.replace('/', '_');
    logPath_ = QString("%1/%2_%3.log").arg(loader.getLogsDirectory()).arg(configurationName_).arg(tmp);
    LOGDEBUG << "logPath: " << logPath_ << std::endl;

    // Attempt to run a new `runner` instance to host an algorithm on a remote host. We use SSH to establish
    // connectivity with the following flags:
    //
    // - -T -- don't allocate a TTY
    // - -f -- make SSH run in background
    // - -n -- don't read from stdin
    // - -x -- disable X11 display forwarding
    //
    // We then run `nohup` so that the remote `runner` process will stay alive even if the SSH connection
    // terminates. The arguments to `nohup` are:
    //
    // - ${SIDECAR}/bin/startup -- a shell script which sets up the environment for running `runner`
    // - -L LOGPATH -- tell `startup` script where to write log output
    // - `runner` -- the command to spawn from within `startup`
    // - OPTS -- option settings to hand to `runner`
    // - NAME -- the name of the processing stream to execute in `runner`
    // - CFG -- the path of the configuration file to use that defines the NAME stream
    //
    remoteCommand_ = QString("ssh -Tfnx %1 "
                             "'/usr/bin/nohup "
                             "\"%2\" "
                             "-L \"%3\" "
                             "runner %4 \"%5\" \"%6\"'")
        .arg(cfg_.host)
        .arg(QString(Utils::FilePath("${SIDECAR}/bin/startup").c_str()))
        .arg(logPath_)
        .arg(cfg_.opts)
        .arg(cfg_.name)
        .arg(loader.getConfigurationPath());

    LOGDEBUG << "remoteCommand: " << remoteCommand_ << std::endl;
}

void
RunnerConfig::setServiceName(const QString& serviceName)
{
    // The service name was changed due to a naming conflict.
    //
    Logger::ProcLog log("setServiceName", Log());
    LOGWARNING << "new service name: " << serviceName << std::endl;
    serviceName_ = serviceName;

    QRegExp re("\\s*\\(\\d+\\)$");
    int pos = re.indexIn(cfg_.name);
    LOGDEBUG << "pos: " << pos << std::endl;
    if (pos != -1) {
        cfg_.name.truncate(pos);
        LOGWARNING << "original runner name: " << cfg_.name << std::endl;
    }

    pos = re.indexIn(serviceName);
    LOGDEBUG << "pos: " << pos << std::endl;
    if (pos != -1) {
        cfg_.name += re.capturedTexts()[0];
        LOGWARNING << "revised runner name: " << cfg_.name << std::endl;
    }
}
