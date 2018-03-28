#include "QtCore/QDir"
#include "QtCore/QFile"
#include "QtCore/QFileInfo"
#include "QtCore/QString"
#include "QtCore/QStringList"
#include "QtXml/QDomDocument"
#include "QtXml/QDomElement"

#include "GUI/LogUtils.h"
#include "IO/ProcessingState.h"
#include "Messages/RadarConfig.h"
#include "Utils/FilePath.h"

#include "Loader.h"
#include "RunnerConfig.h"

using namespace SideCar;
using namespace SideCar::IO;
using namespace SideCar::Configuration;

static const char* const kDefaultLogsDirectory = "/tmp";
static const char* const kDefaultRecordingsDirectory = "/space1/recordings";
static const char* const kDefaultInitialProcessingState = "Run";

static const char* const kEntitySidecar = "sidecar";
static const char* const kEntityRadar = "radar";
static const char* const kAttributeFile = "file";
static const char* const kEntityDP = "dp";
static const char* const kAttributeLogsDirectory = "logsDirectory";
static const char* const kAttributeRecordingsDirectory = "recordingsDirectory";
static const char* const kAttributeLoggerConfiguration = "loggerConfiguration";
static const char* const kEntityRunner = "runner";
static const char* const kAttributeName = "name";
static const char* const kAttributeOpts = "opts";
static const char* const kAttributeHost = "host";
static const char* const kAttributeMulticast = "multicast";
static const char* const kAttributeScheduler = "scheduler";
static const char* const kAttributePriority = "priority";
static const char* const kAttributeCpuAffinity = "cpu";
static const char* const kAttributeInitialProcessingState = "state";
static const char* const kEntityStream = "stream";

static void
expandEnvVars(QString& val)
{
    Utils::FilePath tmp(val.toUtf8().constData());
    val = tmp.c_str();
}

/** Private implementation class the performs the work for the Loader class.
 */
struct Loader::Private {
    static Logger::Log& Log();

    Private(const Loader& parent);

    bool load(const QString& configurationPath);

    void updateInit(RunnerConfig::Init& cfg, QDomElement xml);

    QDomElement loadIncludeFile(const QString& path, const QString& entity, QDomDocument& doc);

    RunnerConfig* getRunnerConfig(const QString& runnerName) const;

    bool finishLoad(Loader::LoadResult result);

    void reset();

    bool getStreams(const QDir& dir, const QDomElement& runner, QList<QDomElement>& streams);

    const Loader& parent_;
    QString configurationPath_;
    QString configurationName_;
    LoadResult loadResult_;
    RunnerConfigList runnerConfigs_;
    QString filePath_;
    QString errorText_;
    int errorLine_;
    int errorColumn_;
    QString logsDirectory_;
    QString recordingsDirectory_;
    QString loggerConfiguration_;
    QString initialProcessingState_;
    QStringList includePaths_;
    QStringList hostNames_;
};

Logger::Log&
Loader::Private::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Configuration.Loader.Private");
    return log_;
}

Loader::Private::Private(const Loader& parent) :
    parent_(parent), configurationPath_(""), configurationName_(""), loadResult_(Loader::kNotLoaded), runnerConfigs_(),
    filePath_(), errorText_(""), errorLine_(-1), errorColumn_(-1), logsDirectory_(""), recordingsDirectory_(""),
    loggerConfiguration_(""), initialProcessingState_(kDefaultInitialProcessingState), includePaths_(), hostNames_()
{
    ;
}

void
Loader::Private::reset()
{
    loadResult_ = Loader::kNotLoaded;
    foreach (RunnerConfig* config, runnerConfigs_) {
        delete config;
    }
    runnerConfigs_.clear();
    filePath_ = "";
    errorText_ = "";
    errorLine_ = -1;
    errorColumn_ = -1;
    logsDirectory_ = "";
    recordingsDirectory_ = "";
    loggerConfiguration_ = "";
    initialProcessingState_ = kDefaultInitialProcessingState;
    includePaths_.clear();
    hostNames_.clear();
}

RunnerConfig*
Loader::Private::getRunnerConfig(const QString& runnerName) const
{
    foreach (RunnerConfig* config, runnerConfigs_) {
        if (config->getRunnerName() == runnerName) { return config; }
    }

    return 0;
}

bool
Loader::Private::finishLoad(Loader::LoadResult result)
{
    loadResult_ = result;
    return loadResult_ == Loader::kOK;
}

void
Loader::Private::updateInit(RunnerConfig::Init& cfg, QDomElement xml)
{
    cfg.name = xml.attribute(kAttributeName, cfg.name);
    cfg.opts = xml.attribute(kAttributeOpts, cfg.opts);
    cfg.host = xml.attribute(kAttributeHost, cfg.host);
    cfg.multicastAddress = xml.attribute(kAttributeMulticast, cfg.multicastAddress);
    cfg.scheduler = xml.attribute(kAttributeScheduler, cfg.scheduler);
    cfg.priority = xml.attribute(kAttributePriority, cfg.priority);
    cfg.cpuAffinity = xml.attribute(kAttributeCpuAffinity, cfg.cpuAffinity);
    cfg.initialProcessingState = xml.attribute(kAttributeInitialProcessingState, cfg.initialProcessingState);
}

bool
Loader::Private::load(const QString& configurationPath)
{
    Logger::ProcLog log("load", Log());

    // Clear out any cached values from the last time we were called.
    //
    reset();
    configurationPath_ = configurationPath;
    configurationName_ = QFileInfo(configurationPath).baseName();
    filePath_ = configurationPath_;

    LOGDEBUG << "configurationName: " << configurationName_ << std::endl;

    // Open the XML file
    //
    QFile file(configurationPath_);
    if (!file.open(QFile::ReadOnly | QFile::Text)) { return finishLoad(Loader::kFailedFileOpen); }

    // Load the XML data, creating a DOM object.
    //
    QDomDocument dom;
    if (!dom.setContent(&file, false, &errorText_, &errorLine_, &errorColumn_)) {
        return finishLoad(Loader::kFailedXMLParse);
    }

    // We MUST have a <sidecar> element
    //
    QDomElement sidecar = dom.namedItem(kEntitySidecar).toElement();
    if (sidecar.isNull()) { return finishLoad(Loader::kMissingSidecarNode); }

    // We MUST have a <radar> configuration element, though it may include
    // another file.
    //
    QDomElement radar = sidecar.namedItem(kEntityRadar).toElement();
    if (radar.isNull()) { return finishLoad(Loader::kMissingRadarNode); }

    QDir dir(QFileInfo(configurationPath).dir());
    if (radar.hasAttribute(kAttributeFile)) {
        QDomDocument doc;
        radar = loadIncludeFile(dir.filePath(radar.attribute(kAttributeFile)), kEntityRadar, doc);
    }

    if (!Messages::RadarConfig::Load(radar)) { return finishLoad(Loader::kInvalidRadarNode); }

    // We MUST have a <dp> (data-processor) element
    //
    QDomElement dp = sidecar.namedItem(kEntityDP).toElement();
    if (dp.isNull()) { return finishLoad(Loader::kMissingDPNode); }

    // Obtain some configuration attributes from the <dp> element
    //
    if (dp.hasAttribute("loggerConfigPath")) {
        loggerConfiguration_ = dp.attribute("loggerConfigPath");
        LOGWARNING << "using deprecated 'loggerConfigPath' attribute instead "
                   << "of '" << kAttributeLoggerConfiguration << "'" << std::endl;
    } else {
        loggerConfiguration_ = dp.attribute(kAttributeLoggerConfiguration);
    }

    expandEnvVars(loggerConfiguration_);
    LOGDEBUG << "loggerConfiguration: " << loggerConfiguration_ << std::endl;

    logsDirectory_ = dp.attribute(kAttributeLogsDirectory, kDefaultLogsDirectory);
    if (logsDirectory_.isEmpty()) {
        LOGWARNING << "using '/tmp' for 'logsDirectory'" << std::endl;
        logsDirectory_ = "/tmp";
    }

    LOGDEBUG << "logsDirectory: " << logsDirectory_ << std::endl;

    if (dp.hasAttribute("recordingBasePath")) {
        recordingsDirectory_ = dp.attribute("recordingBasePath");
        LOGWARNING << "using deprecated 'recordingBasePath' attribute instead "
                   << "of '" << kAttributeRecordingsDirectory << "'" << std::endl;
    } else {
        recordingsDirectory_ = dp.attribute(kAttributeRecordingsDirectory, kDefaultRecordingsDirectory);
    }

    expandEnvVars(recordingsDirectory_);
    LOGDEBUG << "recordingsDirectory: " << recordingsDirectory_ << std::endl;

    initialProcessingState_ = dp.attribute(kAttributeInitialProcessingState, initialProcessingState_);
    if (ProcessingState::GetValue(initialProcessingState_.toStdString()) == ProcessingState::kInvalid) {
        LOGWARNING << "unknown processing state - " << dp.attribute(kAttributeInitialProcessingState)
                   << " - using 'Run'" << std::endl;
        initialProcessingState_ = kDefaultRecordingsDirectory;
    }

    // Load in the <runner> elements
    //
    QDomElement runner = dp.firstChildElement(kEntityRunner);
    while (!runner.isNull()) {
        // Start with some default values.
        //
        RunnerConfig::Init cfg;
        cfg.initialProcessingState = initialProcessingState_;
        if (runner.hasAttribute(kAttributeFile)) {
            // Load in an include file with the runner definition
            //
            QDomDocument doc;
            QDomElement other = loadIncludeFile(dir.filePath(runner.attribute(kAttributeFile)), kEntityRunner, doc);
            if (other.isNull()) { return false; }

            // Fetch the attributes from the include file, overriding the default values above.
            //
            updateInit(cfg, other);

            // Load the <stream> definitions from the include file.
            //
            if (!getStreams(dir, other, cfg.streams) || cfg.streams.isEmpty()) {
                return finishLoad(Loader::kMissingStreams);
            }
        } else {
            // Load the <stream> definitions from the original file.
            //
            if (!getStreams(dir, runner, cfg.streams) || cfg.streams.isEmpty()) {
                return finishLoad(Loader::kMissingStreams);
            }
        }

        // Load the attributes from the original runner object, but possibly use values found in an include file
        // if it is not found in the original.
        //
        updateInit(cfg, runner);

        if (cfg.name.isEmpty()) { cfg.name = QString("Runner %1").arg(runnerConfigs_.size() + 1); }

        LOGDEBUG << "name: " << cfg.name << std::endl << "host: " << cfg.host << std::endl;

        if (!hostNames_.contains(cfg.host)) { hostNames_.append(cfg.host); }

        LOGDEBUG << "multicast: " << cfg.multicastAddress << std::endl
                 << "opts: " << cfg.opts << std::endl
                 << "scheduler: " << cfg.scheduler << std::endl
                 << "priority: " << cfg.priority << std::endl
                 << "cpu: " << cfg.cpuAffinity << std::endl;

        // Create a new RunnerConfig object to represent the found attributes and objects
        //
        runnerConfigs_.append(new RunnerConfig(parent_, cfg));

        // Move to the next <runner> element in the original document.
        //
        runner = runner.nextSiblingElement(kEntityRunner);

    } // Finished loading in runner configurations

    if (runnerConfigs_.isEmpty()) { return finishLoad(Loader::kMissingRunners); }

    return finishLoad(Loader::kOK);
}

bool
Loader::Private::getStreams(const QDir& dir, const QDomElement& runner, QList<QDomElement>& streams)
{
    QDomElement stream = runner.namedItem(kEntityStream).toElement();
    while (!stream.isNull()) {
        if (stream.hasAttribute(kAttributeFile)) {
            // Read in the <stream> stanza from another file.
            //
            QDomDocument doc;
            QDomElement other = loadIncludeFile(dir.filePath(stream.attribute(kAttributeFile)), kEntityStream, doc);
            if (other.isNull()) { return false; }

            streams.push_back(other);
        } else {
            streams.push_back(stream);
        }
        stream = stream.nextSiblingElement(kEntityStream);
    }
    return true;
}

QDomElement
Loader::Private::loadIncludeFile(const QString& path, const QString& entity, QDomDocument& doc)
{
    Logger::ProcLog log("loadIncludeFile", Log());
    LOGINFO << "path: " << path << " entity: " << entity << std::endl;

    filePath_ = path;
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        LOGERROR << "failed to open file '" << path << "'" << std::endl;
        finishLoad(Loader::kFailedFileOpen);
        return QDomElement();
    }

    includePaths_.append(path);

    if (!doc.setContent(&file, false, &errorText_, &errorLine_, &errorColumn_)) {
        LOGERROR << "failed to parse XML file" << std::endl;
        finishLoad(Loader::kFailedXMLParse);
        return QDomElement();
    }

    QDomElement node = doc.namedItem(entity).toElement();
    if (node.isNull()) {
        LOGERROR << "missing entity node" << std::endl;
        finishLoad(Loader::kMissingEntityNode);
        return QDomElement();
    }

    return node;
}

Loader::Loader() : p_(new Private(*this))
{
    ;
}

Loader::~Loader()
{
    ;
}

bool
Loader::load(const QString& configurationPath)
{
    return p_->load(configurationPath);
}

Loader::LoadResult
Loader::getLastLoadResult() const
{
    return p_->loadResult_;
}

const Loader::RunnerConfigList&
Loader::getRunnerConfigs() const
{
    return p_->runnerConfigs_;
}

size_t
Loader::getNumRunnerConfigs() const
{
    return p_->runnerConfigs_.size();
}

RunnerConfig*
Loader::getRunnerConfig(const QString& name) const
{
    return p_->getRunnerConfig(name);
}

RunnerConfig*
Loader::getRunnerConfig(size_t index) const
{
    return p_->runnerConfigs_[index];
}

QString
Loader::getParseFilePath() const
{
    return p_->filePath_;
}

QString
Loader::getParseErrorInfo(int& line, int& column) const
{
    line = p_->errorLine_;
    column = p_->errorColumn_;
    return p_->errorText_;
}

QString
Loader::getConfigurationPath() const
{
    return p_->configurationPath_;
}

QString
Loader::getConfigurationName() const
{
    return p_->configurationName_;
}

QString
Loader::getLoggerConfiguration() const
{
    return p_->loggerConfiguration_;
}

QString
Loader::getLogsDirectory() const
{
    return p_->logsDirectory_;
}

QString
Loader::getRecordingsDirectory() const
{
    return p_->recordingsDirectory_;
}

QStringList
Loader::getIncludePaths() const
{
    return p_->includePaths_;
}

QStringList
Loader::getHostNames() const
{
    return p_->hostNames_;
}
