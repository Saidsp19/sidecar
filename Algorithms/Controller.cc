#include "ace/ACE.h" // for ACE_DLL_PREFIX
#include "ace/Notification_Strategy.h"
#include "ace/Reactor.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#ifdef darwin
#include <dlfcn.h>
#endif

#include "IO/ControlMessage.h"
#include "IO/MessageManager.h"
#include "IO/Module.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/RecordingStateChangeRequest.h"
#include "Logger/Log.h"
#include "Utils/FilePath.h"
#include "XMLRPC/XmlRpcValue.h"

#include "Algorithm.h"
#include "Controller.h"
#include "Recorder.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

/** Helper class for Controller objects that do not spawn a thread for algorithm message processing (see
    Controller::openAndInit()). The story here is that we will receive notify() calls from a Controller's
    message queue when messages are added. We then notify our ACE reactor which will schedule a call to our
    handle_input() call when we are in the main thread, which in turn calls on
    Controller::fetchAndProcessOneMessage() to process the message in the queue.
*/
struct Controller::IncomingNotifier : public ACE_Event_Handler, public ACE_Notification_Strategy {
    static Logger::Log& Log()
    {
        static Logger::Log& log_ = Logger::Log::Find("SideCar.Algorithms.Controller."
                                                     "IncomingNotifier");
        return log_;
    }

    enum { kEvent = ACE_Event_Handler::READ_MASK };

    IncomingNotifier(Controller* controller) :
        ACE_Event_Handler(controller->reactor()), ACE_Notification_Strategy(this, kEvent), controller_(controller)
    {
        reactor()->register_handler(this, kEvent);
    }

    ~IncomingNotifier()
    {
        Logger::ProcLog log("~IncomingNotifier", Log());
        LOGWARNING << std::endl;
        controller_ = 0;
    }

    void close()
    {
        controller_ = 0;
        reactor()->remove_handler(this, ALL_EVENTS_MASK);
    }

    int notify()
    {
        if (!controller_) return 0;
        return reactor()->notify(this, kEvent);
    }

    int notify(ACE_Event_Handler*, ACE_Reactor_Mask) { return notify(); }

    int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
    {
        if (!controller_) return -1;
        return controller_->fetchAndProcessOneMessage() ? 0 : -1;
    }

    int handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask)
    {
        Logger::ProcLog log("handle_close", Log());
        LOGWARNING << std::endl;
        controller_ = 0;
        return 0;
    }

    Controller* controller_;
};

Logger::Log&
Controller::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Algorithms.Controller");
    return log_;
}

Controller::Ref
Controller::Make()
{
    Ref ref(new Controller);
    ref->setSelf(ref);
    return ref;
}

Controller::Controller() :
    Super(), self_(), algorithmName_(""), algorithm_(), recorders_(),
    logLevel_(LogLevelParameter::Make("logLevel", "Log Level", Logger::Priority::kWarning)),
    recordingEnabled_(Parameter::BoolValue::Make("recordingEnabled", "Recording Enabled", false)), processingStat_(),
    xmlConfiguration_(), recording_(false), statsManaged_(true), threaded_(true), timerThread_()
{
    Logger::ProcLog log("Controller", Log());
    LOGINFO << std::endl;

    // Make the log level runtime parameter an advanced setting, register it, and connect a method to receive
    // notification when the parameter changes.
    //
    logLevel_->setAdvanced(true);
    registerParameter(logLevel_);

    // NOTE: Capturing 'this' here is OK since everything is self-contained, and the logLevel attribute will be
    // destroyed before we will, so no risk of dangling reference.
    //
    logLevel_->connectChangedSignalTo([this](auto& v) { logLevelChanged(v); });

    // The recording runtime parameter is not advanced, and we don't need to know when it changes.
    //
    registerParameter(recordingEnabled_);
}

Controller::~Controller()
{
    Logger::ProcLog log("~Controller", Log());
    LOGINFO << algorithm_.get() << std::endl;
}

bool
Controller::openAndInit(const std::string& algorithmName, const std::string& serviceName, Algorithm* algorithm,
                        long threadFlags, long threadPriority, bool threaded)
{
    Logger::ProcLog log("openAndInit", Log());
    LOGINFO << algorithmName << ' ' << serviceName << ' ' << algorithm << " threadFlags: " << std::hex << threadFlags
            << std::dec << " threadPriority: " << threadPriority << " threaded: " << threaded << std::endl;

    if (!reactor()) reactor(ACE_Reactor::instance());

    threaded_ = threaded;
    algorithmName_ = algorithmName;
    setTaskName(serviceName.size() ? serviceName : algorithmName);

    // Load an algorithm if not provided with one.
    //
    if (algorithm) {
        algorithm_.reset(algorithm);
    } else if (!loadAlgorithm()) {
        LOGERROR << "failed to load algorithm" << std::endl;
        setError("Failed to load algorithm");
        return false;
    }

    // Initialize it
    //
    if (!algorithm_->startup()) {
        LOGERROR << "failed algorithm startup" << std::endl;
        setError("Failed to start algorithm");
        return false;
    }

    if (threaded_) {
        // Start a consumer thread for algorithmm processing
        //
        if (activate(threadFlags, 1, 0, threadPriority) == -1) {
            LOGERROR << "failed to start service thread" << std::endl;
            setError("Failed to start service thread");
            return false;
        }
    } else {
        LOGWARNING << getTaskName() << " running in main thread" << std::endl;

        // Install an IncomingNotifier that will get called when something is added to our message queue. Since
        // the notification happens in the main thread, this will cause algorithm message processing to also
        // happen in the main thread.
        //
        msg_queue()->notification_strategy(new IncomingNotifier(this));
    }

    return true;
}

int
Controller::close(u_long flags)
{
    Logger::ProcLog log("close", Log());
    LOGWARNING << algorithmName_ << ' ' << flags << std::endl;

    Super::close(flags);

    if (flags) {
        // If timerThread is running, shut it down
        //
        LOGDEBUG << "task " << algorithmName_ << " checking to see if timer thread is alive" << std::endl;
        if (timerThread_.joinable()) {
            timerThread_.interrupt();
            timerThread_.join();
        } else {
            LOGDEBUG << "task " << algorithmName_ << " timer thread not alive" << std::endl;
        }

        // Deactivate the input queue for the consumer thread. This will cause the thread to exit its svc()
        // routine and terminate.
        //
        LOGDEBUG << "deactivating message queue" << std::endl;
        msg_queue()->deactivate();
        if (threaded_) {
            LOGDEBUG << "joining algorithm service thread" << std::endl;
            if (wait() == -1) {
                LOGERROR << "failed to join algorithm service thread" << std::endl;
            } else {
                LOGWARNING << "algorithm service thread stopped" << std::endl;
            }
        } else {
            // Remove our IncomingNotifier object that we installed inside openAndInit()
            //
            IncomingNotifier* notifier = static_cast<IncomingNotifier*>(msg_queue()->notification_strategy());
            if (notifier) {
                LOGWARNING << "removing IncomingNotifier" << std::endl;
                msg_queue()->notification_strategy(0);
                notifier->close();
            }
        }

        // Give the algorithm a chance to clean up, and then forget about it.
        //
        if (algorithm_) {
            LOGWARNING << "shutting down algorithm" << std::endl;
            algorithm_->shutdown();
            algorithm_.reset();
        }

        LOGINFO << "shutting down channel recorders" << std::endl;
        while (!recorders_.empty()) {
            delete recorders_.back();
            recorders_.pop_back();
        }
    }

    LOGINFO << algorithmName_ << "- END" << std::endl;

    return 0;
}

/** Prototype for the algorithm factory method that must exist in the DLL.
 */
using AlgorithmMaker = Algorithm* (*)(Controller&, Logger::Log&);

bool
Controller::loadAlgorithm()
{
    Logger::ProcLog log("loadAlgorithm", Log());
    LOGINFO << algorithmName_ << std::endl;

    // Construct the name of the DLL we wish to load
    //
    std::string path(ACE_DLL_PREFIX);
    path += algorithmName_;
    LOGDEBUG << "path: " << path << std::endl;

    // Attempt to load/open the DLL.
    //
    ACE_DLL* dll = new ACE_DLL;
    int rc = dll->open(path.c_str());
    if (rc != 0) {
        LOGERROR << "failed to locate DLL '" << path << "' - " << rc << ' ' << errno << std::endl;
#ifdef darwin
        LOGERROR << dlerror() << std::endl;
#endif
        return false;
    }

    // Create the algorithm factory method that will create an Algorthm object for us.
    //
    std::string makerName(algorithmName_);
    makerName += "Make";
    LOGDEBUG << "makerName: " << makerName << std::endl;

    // Attempt to resolve the factory method
    //
    union {
        void* v;
        AlgorithmMaker p;
    } symbol;
    symbol.v = dll->symbol(makerName.c_str());
    if (!symbol.v) {
        LOGERROR << "failed to obtain factory method '" << makerName << "' from DLL" << std::endl;
        return false;
    }

    // Construct the Logger::Log device name for this algorithm.
    //
    std::string logPath("SideCar.Algorithms.");
    logPath += algorithmName_;

    // Attempt to create an Algorithm object to control.
    //
    Logger::Log& logDevice = Logger::Log::Find(logPath);
    logDevice.setPriorityLimit(logLevel_->getValue());
    algorithm_.reset(symbol.p(*this, logDevice));
    if (!algorithm_) {
        LOGERROR << "failed to create algorithm using factory method '" << makerName << "'" << std::endl;
        return false;
    }

    return true;
}

void
Controller::logLevelChanged(const LogLevelParameter& parameter)
{
    if (algorithm_) { algorithm_->getLog().setPriorityLimit(logLevel_->getValue()); }
}

bool
Controller::send(const Messages::Header::Ref& message, size_t channelIndex)
{
    static Logger::ProcLog log("send", Log());
    LOGINFO << algorithmName_ << std::endl;

    if (statsManaged_) { processingStat_.endProcessing(); }

    // Manage the message reference and send the resulting data blocks down the processing stream.
    //
    IO::MessageManager manager(message);

    // Record what was emitted by the algorithm.
    //
    if (recording_) {
        ACE_Message_Block* data = manager.getMessage();
        if (recorders_[channelIndex]->put(data) == -1) {
            LOGERROR << algorithmName_ << " failed to record data" << std::endl;
            setError("Failed to record data");
            data->release();
        }
    }

    return Super::sendManaged(manager, channelIndex);
}

bool
Controller::injectControlMessage(const IO::ControlMessage& msg)
{
    // Place the control message onto the input queue for the consumer thread to process. NOTE: we own data
    // unless successfully placed in the queue.
    //
    ACE_Message_Block* data = msg.getWrapped();
    if (!deliverControlMessage(data, 0)) {
        data->release();
        setError("Failed to enqueue control message");
        return false;
    }

    return true;
}

bool
Controller::injectProcessingStateChange(IO::ProcessingState::Value state)
{
    return injectControlMessage(IO::ProcessingStateChangeRequest(state));
}

bool
Controller::enterInitializeState()
{
    return Super::enterInitializeState() && algorithm_->reset();
}

bool
Controller::enterAutoDiagnosticsState()
{
    return Super::enterAutoDiagnosticsState() && algorithm_->beginAutoDiag();
}

bool
Controller::enterCalibrateState()
{
    return Super::enterCalibrateState() && algorithm_->beginCalibration();
}

bool
Controller::enterRunState()
{
    return Super::enterRunState() && algorithm_->beginRun();
}

bool
Controller::enterStopState()
{
    return Super::enterStopState() && algorithm_->stop();
}

bool
Controller::processDataMessage(ACE_Message_Block* data)
{
    static Logger::ProcLog log("processDataMessage", Log());
    LOGTIN << algorithmName_ << std::endl;

    if (!algorithm_) {
        LOGERROR << "algorithm is not loaded" << std::endl;
        return false;
    }

    // Don't give the algorithm any data if it is not in an active state (auto-diag, calibrate, or running).
    // Just forget the data and return success.
    //
    if (!isActive()) {
        data->release();
        LOGTOUT << "not active" << std::endl;
        return true;
    }

    // Process the incoming message. If the algorithm reports a failure, we report an error and enter the
    // failure state. Otherwise, we update the processing stats.
    //
    IO::MessageManager mgr(data);
    LOGINFO << "msg type: " << mgr.getNativeMessageType() << std::endl;
    Messages::Header::Ref msg(mgr.getNative());

    processingStat_.beginProcessing();

    if (!algorithm_->process(msg, data->msg_priority())) {
        LOGERROR << "failed to process message" << std::endl;
        if (!hasError()) { setError("Failed to process message"); }
    }

    LOGTOUT << "OK" << std::endl;
    return true;
}

bool
Controller::doClearStatsRequest()
{
    if (!algorithm_) return Super::doClearStatsRequest();
    processingStat_.reset();
    return Super::doClearStatsRequest() && algorithm_->clearStats();
}

bool
Controller::doRecordingStateChange(const IO::RecordingStateChangeRequest& msg)
{
    static Logger::ProcLog log("doRecordingStateChanges", Log());
    LOGINFO << algorithmName_ << " path: " << msg.getValue() << std::endl;

    if (recordingEnabled_->getValue()) {
        if (recorders_.empty()) {
            while (recorders_.empty() || recorders_.size() < getNumOutputChannels()) {
                recorders_.push_back(new Recorder(*this));
            }
        }

        // If starting a recording, fetch the path of the recording directory and add our task name as the file
        // name. Command our Recorder object to start its writer thread.
        //
        if (msg.isOn()) {
            std::string path(msg.getValue());
            path += '/';
            path += getTaskName();
            if (startRecordings(path)) {
                recording_ = true;
                algorithm_->recordingStarted();
            } else {
                setError("Failed to start recorder(s).");
            }
        } else {
            // Shutdown Recorder writer thread.
            //
            recording_ = false;
            stopRecordings();
            algorithm_->recordingStopped();
            LOGINFO << getTaskName() << " recording stopped" << std::endl;
        }
    }

    return true;
}

bool
Controller::startRecordings(const std::string& fileNamePrefix)
{
    Logger::ProcLog log("startRecordings", Log());
    LOGINFO << "prefix: " << fileNamePrefix << std::endl;

    for (size_t index = 0; index < recorders_.size(); ++index) {
        std::string path(fileNamePrefix);
        if (recorders_.size() > 1) {
            std::ostringstream os("");
            os << "-" << (index + 1);
            path += os.str();
        }

        path += ".pri";

        LOGINFO << getTaskName() << " recording into file " << path << " on channel " << index << std::endl;
        if (!recorders_[index]->start(path)) {
            while (index != 0) {
                --index;
                recorders_[index]->stop();
            }
            return false;
        }
    }

    return true;
}

bool
Controller::stopRecordings()
{
    Logger::ProcLog log("stopRecordings", Log());
    LOGINFO << std::endl;

    for (size_t index = 0; index < recorders_.size(); ++index) {
        LOGINFO << getTaskName() << " stopping recording on channel " << index << std::endl;
        recorders_[index]->stop();
    }

    return true;
}

bool
Controller::doParametersChange(const IO::ParametersChangeRequest& request)
{
    Logger::ProcLog log("doParametersChange", Log());
    LOGINFO << std::endl;
    bool ok = true;
    if (algorithm_) {
        algorithm_->beginParameterChanges();
        ok = Super::doParametersChange(request);
        algorithm_->endParameterChanges();
    }

    return ok;
}

bool
Controller::fetchAndProcessOneMessage()
{
    static Logger::ProcLog log("fetchAndProcessOneMessage", Log());
    LOGINFO << "message count: " << msg_queue()->message_count() << std::endl;
    ACE_Message_Block* data;
    if (getq(data) == -1) return false;
    processOneMessage(data);
    return true;
}

void
Controller::processOneMessage(ACE_Message_Block* data)
{
    static Logger::ProcLog log("processOneMessage", Log());
    LOGDEBUG << getTaskName() << " message: " << data << std::endl;
    if (!processMessage(data)) {
        LOGERROR << getTaskName() << " failed to process message" << std::endl;
        setError("Failed to process message");
    }
}

int
Controller::svc()
{
    // Consumer running in its own thread
    //
    Logger::ProcLog log("svc", Log());
    LOGINFO << thr_mgr()->thr_self() << ' ' << getTaskName() << " STARTING" << std::endl;

    // Fetch messages from our input queue, process them, and repeat.
    //
    ACE_Message_Block* data;
    while (getq(data) != -1) processOneMessage(data);

    LOGINFO << thr_mgr()->thr_self() << ' ' << getTaskName() << " EXITING" << std::endl;
    return 0;
}

void
Controller::fillStatus(IO::StatusBase& status)
{
    static Logger::ProcLog log("fillStatus", Log());
    LOGINFO << std::endl;

    // Fill in the given XML status object with information about our current state.
    //
    Super::fillStatus(status);

    // Add our own items to the XML status object.
    //
    status.setSlot(ControllerStatus::kRecordingEnabled, recordingEnabled_->getValue());
    status.setSlot(ControllerStatus::kRecordingOn, recording_);

    // If we are recording, calculate the sum of all of the Recorder message queues.
    //
    int queueCount = 0;
    if (recording_) {
        for (size_t index = 0; index < recorders_.size(); ++index)
            queueCount += recorders_[index]->msg_queue()->message_count();
    }

    status.setSlot(ControllerStatus::kRecordingQueueCount, queueCount);
    status.setSlot(ControllerStatus::kAlgorithmName, algorithmName_);
    status.setSlot(ControllerStatus::kAverageProcessingTime, processingStat_.getAverageProcessingTime());
    status.setSlot(ControllerStatus::kMinimumProcessingTime, processingStat_.getMinimumProcessingTime());
    status.setSlot(ControllerStatus::kMaximumProcessingTime, processingStat_.getMaximumProcessingTime());

    // If the algorithm says that it has some status slots, give it a chance to add them to the XML status
    // object.
    //
    if (getStatusSize() > ControllerStatus::kNumSlots) algorithm_->setInfoSlots(status);
}

size_t
Controller::getStatusSize() const
{
    return algorithm_->getNumInfoSlots();
}

bool
Controller::deliverControlMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    return deliverDataMessage(data, timeout);
}

bool
Controller::deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    static Logger::ProcLog log("deliverDataMessage", Log());
    LOGINFO << algorithmName_ << ' ' << data << ' ' << timeout << std::endl;

    // Add the incoming message to our message queue for our algorithm consumer thread. NOTE: we only take
    // ownership of data if we can put it in the queue.
    //
    if (putq(data, timeout) == -1) {
        LOGERROR << "failed to add message to message queue for " << algorithmName_ << std::endl;
        setError("Failed to enqueue data message");
        return false;
    }

    return true;
}

void
Controller::addProcessingStatSample(const Time::TimeStamp& delta)
{
    processingStat_.addSample(delta);
}

bool
Controller::calculateUsingDataValue() const
{
    return Super::calculateUsingDataValue() || recordingEnabled_->getValue();
}

void
Controller::alarmTimerProc()
{
    Logger::ProcLog log("alarmTimerProc", Log());
    LOGINFO << getTaskName() << std::endl;

    boost::posix_time::seconds period(getTimerSecs());
    boost::system_time now = boost::get_system_time();

    while (1) {
        // Wait until it is time to wakeup.
        //
        boost::system_time wakeupTime = now + period;
        boost::thread::sleep(wakeupTime);

        // Record time here to minimize timer drift from message queue latency.
        //
        now = boost::get_system_time();

        // Send control message to Task to let it know to invoke its alarm handler function in a thread-safe
        // manner.
        //
        ACE_Message_Block* data = IO::MessageManager::MakeControlMessage(IO::ControlMessage::kTimeout, 0);
        if (put(data, 0) == -1) {
            LOGINFO << getTaskName() << " failed to post control message" << std::endl;
            break;
        }
    }

    LOGINFO << getTaskName() << " thread exiting" << std::endl;
}

void
Controller::setTimerSecs(int timerSecs)
{
    static Logger::ProcLog log("setTimerSecs", Log());
    LOGINFO << getTaskName() << " timerSecs: " << timerSecs << std::endl;

    // Stop timer thread if there is one running
    //
    if (timerThread_.joinable()) {
        LOGINFO << getTaskName() << " stopping alarm timer" << std::endl;
        timerThread_.interrupt();
        timerThread_.join();
    }

    timerSecs_ = timerSecs;

    // Start timer thread if valid timer period.
    //
    if (timerSecs > 0) {
        LOGWARNING << getTaskName() << " starting alarm timer" << std::endl;
        timerThread_ = boost::thread(boost::bind(&Controller::alarmTimerProc, this));
    }
}

bool
Controller::doTimeout()
{
    static Logger::ProcLog log("doTimeout", Log());

    if (!algorithm_) {
        LOGERROR << "algorithm is not loaded" << std::endl;
        return false;
    }

    // Only call the algorithm's processAlarm() if we are in an active processing state.
    //
    if (isActive()) algorithm_->processAlarm();

    return true;
}
