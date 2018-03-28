#ifdef linux

#include <fstream>
#include <sstream>

#else

#ifdef darwin

#include <malloc/malloc.h>

#endif

#endif

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "ace/Reactor.h"
#include "ace/Sched_Params.h"

#include "Configuration/RunnerConfig.h"
#include "GUI/LogUtils.h"
#include "IO/ClearStatsRequest.h"
#include "IO/ParametersChangeRequest.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/RecordingStateChangeRequest.h"
#include "IO/Stream.h"
#include "IO/StreamStatus.h"
#include "Logger/ConfiguratorFile.h"
#include "Utils/Format.h"
#include "Utils/Utils.h"
#include "XMLRPC/XmlRpcValue.h"

#include "App.h"
#include "LogCollector.h"
#include "RemoteController.h"
#include "RunnerStatus.h"
#include "StatusEmitter.h"
#include "StreamBuilder.h"

#include "GUI/Utils.h" // NOTE: include after any boost::signal.hpp

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::GUI;
using namespace SideCar::Runner;

const std::string about = "SideCar application that creates and manages one or more processing streams.";

const Utils::CmdLineArgs::OptionDef options[] = {{'d', "debug", "turn on verbose debugging", 0},
                                                 {'L', "logger", "use LOG for logging configuration", "LOG"},
                                                 {'Q', "daq", "setup for data acquisition mode", 0}};

const Utils::CmdLineArgs::ArgumentDef args[] = {{"NAME", "Runner to startup"}, {"CONFIG", "Configuration file"}};

App* App::app_ = 0;

App*
App::GetApp()
{
    return app_;
}

Logger::Log&
App::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("runner.App");
    return log_;
}

extern "C" void
quit(int sig)
{
    Logger::ProcLog log("quit", App::Log());
    LOGERROR << "signal: " << sig << std::endl;
    ACE_Reactor::instance()->end_event_loop();
}

App::App(int argc, char* const* argv) :
    cla_(argc, argv, about, options, sizeof(options), args, sizeof(args)), logCollector_(LogCollector::Make()),
    loggerConfig_(), streams_(), remoteController_(), statusEmitter_(), loader_(), runnerConfig_()
{
    Logger::Log::Root().addWriter(logCollector_);
    Logger::ProcLog log("App", Log());
    LOGINFO << std::endl;
    app_ = this;
}

static void
stackPrefault()
{
    // Allocate 32K of stack space and touch it. This should keep future function calls from paging stack space.
    //
    static const int kMaxSafeStack = 32 * 1024;
    unsigned char dummy[kMaxSafeStack];
    memset(&dummy, 0, sizeof(dummy));
}

void
App::initializeRealTime(const QString& scheduler)
{
    Logger::ProcLog log("initializeRealTime", Log());
    LOGINFO << "scheduler: " << scheduler.toStdString() << std::endl;

    int policy;
    if (scheduler == "SCHED_FIFO") {
        policy = ACE_SCHED_FIFO;
    } else if (scheduler == "SCHED_RR") {
        policy = ACE_SCHED_RR;
    } else if (scheduler == "SCHED_OTHER") {
        policy = ACE_SCHED_OTHER;
    } else {
        Utils::Exception ex("invalid RT scheduler policy specified - ");
        ex << scheduler.toStdString();
        log.thrower(ex);
    }

    int priority = runnerConfig_->getPriority().toInt();
    if (priority == 0) {
        Utils::Exception ex("invalid scheduler priority specified - ");
        ex << priority;
        log.thrower(ex);
    }

    LOGWARNING << "configuring scheduler" << std::endl;
    ACE_Sched_Params schedParams(policy, priority, ACE_SCOPE_PROCESS);

    if (ACE_OS::sched_params(schedParams) != 0) {
        Utils::Exception ex("");
        if (ACE_OS::last_error() == EPERM)
            ex << "User is not priviledged to modify scheduler";
        else
            ex << "Failed to set scheduler params";
        log.thrower(ex);
    }

    LOGINFO << "Policy is    = " << schedParams.policy() << " (0=OTHER, 1=FIFO, 2=RR) " << std::endl;
    LOGINFO << "Priority MIN = " << ACE_Sched_Params::priority_min(policy, ACE_SCOPE_PROCESS) << std::endl;
    LOGINFO << "Priority MAX = " << ACE_Sched_Params::priority_max(policy, ACE_SCOPE_PROCESS) << std::endl;
    LOGINFO << "Priority is  = " << schedParams.priority() << std::endl;

    // Do this before we call mlockall, at least according to mlockall(2) NOTES section.
    //
    LOGWARNING << "prefaulting the stack." << std::endl;
    stackPrefault();

    LOGWARNING << "calling mlockall to prevent page faults" << std::endl;
    if (::mlockall(MCL_CURRENT | MCL_FUTURE) == -1) { LOGERROR << "mlockall failed" << std::endl; }
}

void
App::initialize()
{
    Logger::ProcLog log("initialize", Log());
    LOGINFO << std::endl;

    // Process command line parameters
    //
    std::string name(cla_.arg(0));
    if (name.size() == 0) {
        Utils::Exception ex("empty rtrunner name");
        log.thrower(ex);
    }

    // Configure logger
    //
    if (cla_.hasOpt("debug")) {
        Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    } else {
        Logger::Log::Root().setPriorityLimit(Logger::Priority::kWarning);
    }

    std::string value;
    if (cla_.hasOpt("logger", value)) {
        loggerConfig_.reset(new Logger::ConfiguratorFile(value));
        loggerConfig_->startMonitor(10);
    }

    // Load XML configuration file
    //
    if (!loader_.load(QString::fromStdString(cla_.arg(1)))) {
        Utils::Exception ex("failed to load file ");
        ex << cla_.arg(1);
        ex << " - " << loader_.getLastLoadResult();
        log.thrower(ex);
    }

    // Get XML configuration info for Runner thread
    //
    runnerConfig_.reset(loader_.getRunnerConfig(QString::fromStdString(name)));
    if (!runnerConfig_) {
        Utils::Exception ex("failed to locate runner ");
        ex << cla_.arg(0) << " in configuration file";
        log.thrower(ex);
    }

    // If Scheduler is specified as an RT scheduler (FIFO or RR) attempt to configure the scheduler
    //
    QString scheduler = runnerConfig_->getScheduler();
    if (scheduler != "SCHED_INHERIT") { initializeRealTime(scheduler); }

    struct sigaction action;

#if 0
  ::memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_IGN;
  ::sigaction(SIGABRT, &action, 0);
    
  ::memset(&action, 0, sizeof(action));
  action.sa_handler = &quit;
  ::sigaction(SIGSEGV, &action, 0);
#endif

    ::memset(&action, 0, sizeof(action));
    action.sa_handler = &quit;
    ::sigaction(SIGTERM, &action, 0);

    ::memset(&action, 0, sizeof(action));
    action.sa_handler = &quit;
    ::sigaction(SIGINT, &action, 0);

    ACE_Reactor::instance()->restart(1);

    statusEmitter_ = StatusEmitter::Make(*this);
    statusEmitter_->open(THR_INHERIT_SCHED, 0);

    std::string multicastAddress = runnerConfig_->getMulticastAddress().toStdString();
    foreach (QDomElement stream, runnerConfig_->getStreamNodes()) {
        streams_.push_back(StreamBuilder::Make(stream, statusEmitter_, multicastAddress));
    }

#ifdef linux
    std::ostringstream os;
    os << "/proc/" << ::getpid() << "/statm";
    statmPath_ = os.str();
#endif
    postControlMessage(IO::ProcessingStateChangeRequest(
                           IO::ProcessingState::GetValue(runnerConfig_->getInitialProcessingState().toStdString()))
                           .getWrapped());
}

App::~App()
{
    Logger::Log::Root().removeWriter(logCollector_);
}

void
App::run()
{
    Logger::ProcLog log("run", Log());
    LOGINFO << std::endl;

    remoteController_.reset(new RemoteController(*this));
    remoteController_->start(runnerConfig_->getServiceName(), THR_INHERIT_SCHED, 0);

    ACE_Reactor::instance()->run_reactor_event_loop();

    LOGWARNING << runnerConfig_->getServiceName() << " shutting down" << std::endl;

    remoteController_->stop();

    for (auto v : streams_) v->close();
    streams_.clear();
}

void
App::shutdown()
{
    Logger::ProcLog log("shutdown", Log());
    LOGWARNING << "gracefully" << std::endl;
    statusEmitter_->close();

    // !!! NOTE: do not add any calls after this one. A race condition will exist between this thread and the
    // !!! main thread running inside run(). The only safe thing to do is to return. The RemoteControlleBase
    // !!! class guarantees the XML-RPC server will remain alive while there are active XML-RPC requests such as
    // !!! this one.
    //
    ACE_Reactor::instance()->end_event_loop();
}

void
App::recordingStateChange(const std::string& dirPath)
{
    Logger::ProcLog log("recordingStateChange", Log());
    LOGINFO << dirPath << std::endl;
    postControlMessage(IO::RecordingStateChangeRequest(dirPath).getWrapped());
    statusEmitter_->emitStatus();
}

void
App::clearStats()
{
    Logger::ProcLog log("clearStats", Log());
    LOGINFO << std::endl;
    postControlMessage(IO::ClearStatsRequest().getWrapped());
}

void
App::postControlMessage(ACE_Message_Block* data)
{
    Logger::ProcLog log("put", Log());
    LOGINFO << data->msg_type() << ' ' << data->size() << std::endl;

    for (size_t index = 0; index < streams_.size(); ++index) {
        std::unique_ptr<ACE_Message_Block> tmp(data->duplicate());
        if (streams_[index]->put(tmp.get()) == -1) {
            LOGERROR << "failed to post message to stream " << streams_[index]->getName() << std::endl;
        } else {
            tmp.release();
        }
    }

    data->release();
}

void
App::fillStatus(XmlRpc::XmlRpcValue& status)
{
    static Logger::ProcLog log("fillStatus", Log());
    LOGINFO << streams_.size() << std::endl;

    std::unique_ptr<XmlRpc::XmlRpcValue::ValueArray> streamStatusArray(new XmlRpc::XmlRpcValue::ValueArray);
    for (auto s : streams_) {
        IO::StreamStatus streamStatus(s->getName());
        s->fillStatus(streamStatus);
        streamStatusArray->push_back(streamStatus.getXMLData());
    }

    double memoryUsed = 0;

#ifdef linux

    // Fetch data from /proc/PID/statm, where PID is our process ID.
    //
    std::ifstream fs(statmPath_.c_str());
    int size, resident, share, text, lib, data, dt;
    fs >> size >> resident >> share >> text >> lib >> data >> dt;

    // According to gnome-system-monitor, the amount of writable memory (allocated data) available to a runner
    // is equal to resident - shared. Since these are page counts, multiply by our page size to get byte counts.
    // Let the RunnerItem class in the Master application convert the byte count into a suitable display format.
    //
    memoryUsed = (resident - share) * 4096.0;

#endif

#ifdef darwin
    malloc_statistics_t t = {0, 0, 0, 0};
    malloc_zone_statistics(0, &t);
    memoryUsed = t.size_allocated;
#endif

    RunnerStatus::Make(status, *runnerConfig_, std::move(streamStatusArray), std::move(logCollector_->dump()),
                       memoryUsed);
}

void
App::getChangedParameters(XmlRpc::XmlRpcValue& value) const
{
    static Logger::ProcLog log("getChangedParameters", Log());
    LOGINFO << "size: " << streams_.size() << std::endl;
    value.setSize(streams_.size());
    for (size_t index = 0; index < streams_.size(); ++index) {
        XmlRpc::XmlRpcValue changes;
        streams_[index]->getChangedParameters(changes);
        LOGDEBUG << "changes: " << changes.toXml() << std::endl;
        value[index] = changes;
    }

    LOGDEBUG << "value: " << value.toXml() << std::endl;
}

bool
App::getParameters(int streamIndex, int taskIndex, XmlRpc::XmlRpcValue& result) const
{
    static Logger::ProcLog log("getParameters", Log());
    LOGINFO << streamIndex << ' ' << taskIndex << std::endl;
    if (streamIndex < 0 || streamIndex >= int(streams_.size())) return false;
    IO::Stream::Ref stream(streams_[streamIndex]);
    IO::Task::Ref task(stream->getTask(taskIndex));
    if (!task) return false;
    LOGDEBUG << "task: " << task->getTaskName() << std::endl;
    task->getCurrentParameters(result);
    return true;
}

bool
App::setParameters(int streamIndex, int taskIndex, const XmlRpc::XmlRpcValue& params) const
{
    static Logger::ProcLog log("setParameters", Log());
    LOGINFO << streamIndex << ' ' << taskIndex << std::endl;

    IO::Stream::Ref stream(streams_[streamIndex]);
    IO::Task::Ref task(stream->getTask(taskIndex));
    LOGINFO << "task: " << task->getTaskName() << std::endl;
    IO::ParametersChangeRequest request(params, false);
    ACE_Message_Block* data = request.getWrapped();

    // !!! Place the parameter change message at the front of the queue, so that it will take place before other
    // !!! data messages.
    //
    if (task->msg_queue()->enqueue_head(data) == -1) {
        data->release();
        return false;
    }

    return true;
}

void
App::setServiceName(const std::string& serviceName)
{
    Logger::ProcLog log("setServiceName", Log());
    LOGWARNING << "new service name: " << serviceName << std::endl;
    runnerConfig_->setServiceName(QString::fromStdString(serviceName));
    if (remoteController_) {
        remoteController_->stop();
        remoteController_->start(runnerConfig_->getServiceName());
    }
}
