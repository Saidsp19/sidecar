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

    if (cfg_.host == "localhost") {
        remoteCommand_ =
            QString("runner %1 \"%2\" \"%3\"").arg(cfg_.opts).arg(cfg_.name).arg(loader.getConfigurationPath());
    } else {
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
    }
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
