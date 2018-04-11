#include <sstream>
#include <typeinfo>

#include "ace/Guard_T.h"
#include "ace/Message_Queue_T.h"
#include "ace/Reactor.h"

#include "Logger/Log.h"

#include "MessageManager.h"
#include "ParametersChangeRequest.h"
#include "ProcessingStateChangeRequest.h"
#include "RecordingStateChangeRequest.h"
#include "Stream.h"
#include "Task.h"
#include "TaskStatus.h"

using namespace SideCar;
using namespace SideCar::IO;
using namespace SideCar::Messages;

const char* const*
Task::ProcessingStateEnumTraits::GetEnumNames()
{
    return ProcessingState::GetNames() + GetMinValue();
}

Logger::Log&
Task::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Task");
    return log_;
}

Task::Task(bool usingData) :
    Super(), stream_(), taskName_(""), error_(""), taskIndex_(0), inputs_(), inputStats_(), outputs_(),
    parameterMap_(), parameterVector_(),
    processingStateParameter_(ProcessingStateParameter::Make("processingState", "Processing State",
                                                             ProcessingStateParameter::None())),
    editingEnabled_(Parameter::BoolValue::Make("editingEnabled", "Editing Enabled", true)), connectionInfo_(""),
    processingState_(ProcessingState::kInvalid), lastProcessingState_(ProcessingState::kInvalid),
    alwaysUsingData_(Parameter::BoolValue::Make("alwaysUsingData", "Always Using Data", false)), threadParams_(),
    usingData_(usingData)
{
    static Logger::ProcLog log("Task", Log());
    LOGINFO << this << std::endl;

    // Receive notification when the processing state runtime parameter changes.
    //
    processingStateParameter_->connectChangedSignalTo(boost::bind(&Task::processingStateParameterChanged, this,
                                                                  _1));

    // Make these parameters 'advanced' so that they don't normally show up in the Master parameter setting
    // editor.
    //
    processingStateParameter_->setAdvanced(true);
    alwaysUsingData_->setAdvanced(true);

    // Make editingEnabled invisible to the Master application.
    //
    editingEnabled_->setEditable(false);
    registerParameter(processingStateParameter_);
    registerParameter(editingEnabled_);
    registerParameter(alwaysUsingData_);

    // Record the number of parameter values associated with the Task class. The Master application will not
    // show the parameter editor unless an object has additional runtime parameter values. !!! Hacky...
    //
    taskParameterCount_ = parameterVector_.size();

    // Increase the buffering on the message queue held by the Task. The default (16K) is too small for our
    // message sizes and work-loads.
    //
    ACE_Message_Queue<ACE_MT_SYNCH>* queue = msg_queue();
    if (queue) {
        static const int kQueueSize = 10 * 1024 * 1024; // 10M
        queue->high_water_mark(kQueueSize);
        queue->low_water_mark(kQueueSize - 1);
    }

    // Make space for one default channel. This is to support old code, such as some unit tests.
    //
    inputStats_.push_back(Stats());
}

Task::~Task()
{
    static Logger::ProcLog log("~Task", Log());
    LOGINFO << this << std::endl;
}

void
Task::setError(const std::string& error, bool force)
{
    Logger::ProcLog log("setError", Log());

    if (error_.size() == 0 || force) {
        error_ = error;
        if (!stream_.expired()) { LOGERROR << "task: " << taskName_ << " error: " << error << std::endl; }
    }

    enterProcessingState(ProcessingState::kFailure);
}

bool
Task::registerParameter(const Parameter::Ref& parameter)
{
    Logger::ProcLog log("registerParameter", Log());
    LOGINFO << parameter->getName() << std::endl;

    if (parameterMap_.find(parameter->getName()) != parameterMap_.end()) {
        LOGERROR << "duplicate registration of a parameter with name '" << parameter->getName() << "'"
                 << std::endl;
        return false;
    }

    parameterMap_[parameter->getName()] = parameterVector_.size();
    parameterVector_.push_back(parameter);

    LOGINFO << "OK" << std::endl;
    return true;
}

bool
Task::unregisterParameter(const Parameter::Ref& parameter)
{
    Logger::ProcLog log("unregisterParameter", Log());
    LOGINFO << parameter->getName() << std::endl;

    ParameterMap::iterator pos = parameterMap_.find(parameter->getName());
    if (pos == parameterMap_.end()) {
        LOGERROR << "no registration of a parameter with name '" << parameter->getName() << "'" << std::endl;
        return false;
    }

    parameterVector_.erase(parameterVector_.begin() + pos->second);
    parameterMap_.erase(pos);

    return true;
}

/** Transition matrix for processing state changes. First index is the goal processing state, and the second is
    the current processing state. The value stored is the next processing state to go into.
*/
static ProcessingState::Value transitions_[ProcessingState::kNumStates][ProcessingState::kNumStates] = {
    // *** kInvalid goal state (not really a valid state to move to). Stay in invalid sate
    //
    {
        // * CURRENT *
        ProcessingState::kInvalid, // kInvalid
        ProcessingState::kInvalid, // kInitialize
        ProcessingState::kInvalid, // kAutoDiagnostic
        ProcessingState::kInvalid, // kCalibrate
        ProcessingState::kInvalid, // kRun
        ProcessingState::kInvalid, // kStop
        ProcessingState::kInvalid  // kFailure
    },
    // *** kInitialize goal state
    //
    {
        // * CURRENT *
        ProcessingState::kInitialize, // kInvalid
        ProcessingState::kInitialize, // kInitialize
        ProcessingState::kStop,       // kAutoDiagnostic
        ProcessingState::kStop,       // kCalibrate
        ProcessingState::kStop,       // kRun
        ProcessingState::kInitialize, // kStop
        ProcessingState::kInitialize  // kFailure
    },
    // *** kAutoDiagnostic goal state
    //
    {
        // * CURRENT *
        ProcessingState::kInitialize,     // kInvalid
        ProcessingState::kAutoDiagnostic, // kInitialize
        ProcessingState::kStop,           // kAutoDiagnostic
        ProcessingState::kStop,           // kCalibrate
        ProcessingState::kStop,           // kRun
        ProcessingState::kInitialize,     // kStop
        ProcessingState::kInitialize      // kFailure
    },
    // *** kCalibrate goal state
    //
    {
        // * CURRENT *
        ProcessingState::kInitialize, // kInvalid
        ProcessingState::kCalibrate,  // kInitialize
        ProcessingState::kStop,       // kAutoDiagnostic
        ProcessingState::kCalibrate,  // kCalibrate
        ProcessingState::kStop,       // kRun
        ProcessingState::kInitialize, // kStop
        ProcessingState::kInitialize  // kFailure
    },
    // *** kRun goal state
    //
    {
        // * CURRENT *
        ProcessingState::kInitialize, // kInvalid
        ProcessingState::kRun,        // kInitialize
        ProcessingState::kStop,       // kAutoDiagnostic
        ProcessingState::kStop,       // kCalibrate
        ProcessingState::kRun,        // kRun
        ProcessingState::kInitialize, // kStop
        ProcessingState::kInitialize  // kFailure
    },
    // *** kStop goal state
    //
    {
        // * CURRENT *
        ProcessingState::kInitialize, // kInvalid
        ProcessingState::kStop,       // kInitialize
        ProcessingState::kStop,       // kAutoDiagnostic
        ProcessingState::kStop,       // kCalibrate
        ProcessingState::kStop,       // kRun
        ProcessingState::kStop,       // kStop
        ProcessingState::kInitialize  // kFailure
    },
    // *** kFailure goal state
    //
    {
        // * CURRENT *
        ProcessingState::kFailure, // kInvalid
        ProcessingState::kFailure, // kInitialize
        ProcessingState::kFailure, // kAutoDiagnostic
        ProcessingState::kFailure, // kCalibrate
        ProcessingState::kFailure, // kRun
        ProcessingState::kFailure, // kStop
        ProcessingState::kFailure  // kFailure
    }};

bool
Task::enterProcessingState(ProcessingState::Value goalState)
{
    static Logger::ProcLog log("enterProcessingState", Log());

    LOGINFO << "task: " << taskName_ << " current: " << ProcessingState::GetName(processingState_) << '/'
            << processingState_ << " goal: " << ProcessingState::GetName(goalState) << '/'
            << goalState << std::endl;

    // Remember the new processing state for use with enterLastProcessingState()
    //
    if (ProcessingState::IsNormal(goalState)) { lastProcessingState_ = goalState; }

    // Keep looping as long as we have not encountered a failure, until we've reached the desired end state.
    //
    bool ok = true;
    while (goalState != processingState_) {
        LOGDEBUG << "goalState: " << goalState << " processingState: " << processingState_ << std::endl;

        ProcessingState::Value nextState = transitions_[goalState][processingState_];

        LOGDEBUG << "current: " << ProcessingState::GetName(processingState_)
                 << " next: " << ProcessingState::GetName(nextState) << std::endl;

        switch (nextState) {
        case ProcessingState::kInitialize: ok = enterInitializeState(); break;
        case ProcessingState::kAutoDiagnostic: ok = enterAutoDiagnosticsState(); break;
        case ProcessingState::kCalibrate: ok = enterCalibrateState(); break;
        case ProcessingState::kRun: ok = enterRunState(); break;
        case ProcessingState::kStop: ok = enterStopState(); break;
        default: // Covers kInvalid and kFailure
            ok = false;
            break;
        }

        if (!ok) break;

        processingState_ = nextState;
    }

    if (!ok) {
        // We failed somewhere above. Enter the failure state.
        //
        LOGDEBUG << "entering FAILURE state" << std::endl;
        if (!hasError()) setError("Unknown");
        enterFailureState();
    }

    // Update the runtime parameter value with any changes. Only a subset of the values are valid for the user
    // to pick from.
    //
    if (processingState_ < ProcessingStateEnumTraits::GetMinValue() ||
        processingState_ > ProcessingStateEnumTraits::GetMaxValue()) {
        processingStateParameter_->setValue(ProcessingStateEnumDef::None());
    } else {
        processingStateParameter_->setValue(processingState_);
    }

    LOGDEBUG << "new state: " << ProcessingState::GetName(processingState_) << std::endl;

    updateUsingDataValue();

    return true;
}

bool
Task::enterInitializeState()
{
    Logger::ProcLog log("enterInitializeState", Log());
    LOGINFO << taskName_ << std::endl;

    // Clear the message stats and error text.
    //
    resetProcessedStats();
    clearError();
    return true;
}

bool
Task::enterAutoDiagnosticsState()
{
    Logger::ProcLog log("enterAutoDiagnosticsState", Log());
    LOGINFO << taskName_ << std::endl;
    return true;
}

bool
Task::enterCalibrateState()
{
    Logger::ProcLog log("enterCalibrateState", Log());
    LOGINFO << taskName_ << std::endl;
    return true;
}

bool
Task::enterRunState()
{
    Logger::ProcLog log("enterRunState", Log());
    LOGINFO << taskName_ << std::endl;
    return true;
}

bool
Task::enterStopState()
{
    Logger::ProcLog log("enterStopState", Log());
    LOGINFO << taskName_ << std::endl;
    return true;
}

bool
Task::enterFailureState()
{
    if (processingState_ == ProcessingState::kFailure) return true;
    Logger::ProcLog log("enterFailureState", Log());
    LOGINFO << "task: " << taskName_ << std::endl;

    // Make sure we visit the stop state before we go into failure
    //
    if (processingState_ != ProcessingState::kStop) enterStopState();

    processingState_ = ProcessingState::kFailure;
    return true;
}

bool
Task::enterLastProcessingState()
{
    Logger::ProcLog log("enterLastProcessingState", Log());
    LOGINFO << taskName_ << ' ' << lastProcessingState_ << std::endl;
    clearError();

    if (lastProcessingState_ == ProcessingState::kInvalid) return true;

    return enterProcessingState(lastProcessingState_);
}

bool
Task::putInChannel(const Messages::Header::Ref& msg, size_t channel)
{
    static Logger::ProcLog log("putInChannel", Log());
    LOGINFO << channel << std::endl;

    MessageManager mgr(msg);
    ACE_Message_Block* data = mgr.getMessage();
    data->msg_priority(channel);
    if (put(data, 0) == -1) {
        data->release();
        return false;
    }

    return true;
}

int
Task::put(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    static Logger::ProcLog log("put", Log());
    LOGINFO << taskName_ << " msg_type: " << data->msg_type() << std::endl;

    if (MessageManager::IsControlMessage(data)) {
        // Forward any control messages down the processing chain. Create a duplicate message because we want to
        // keep ownership of the original. Do not do this for timeout messages, since those are local to this
        // task.
        //
        if (MessageManager::GetControlMessageType(data) != ControlMessage::kTimeout) {
            ACE_Message_Block* dup = data->duplicate();
            if (put_next(dup) == -1) {
                dup->release();
                if (next()) LOGWARNING << "failed put_next() for control message" << std::endl;
            }
        }

        // Now deliver the original to ourselves.
        //
        return deliverControlMessage(data, timeout) ? 0 : -1;
    } else if (MessageManager::IsDataMessage(data)) {
        // Data message delivery. Extract the channel index from the message and use to update the channel
        // stats.
        //
        MessageManager mgr(data->duplicate());
        size_t channelIndex = data->msg_priority();
        Header::Ref msg = mgr.getNative();
        updateInputStats(channelIndex, msg->getSize(), msg->getMessageSequenceNumber());

        // Attempt to deliver the message to the task. For threaded tasks, this will insert the message into a
        // work queue for the thread to process.
        //
        if (!deliverDataMessage(data, timeout)) {
            LOGWARNING << "failed deliverDataMessage()" << std::endl;
            setError("Failed to deliver message to task");
            return -1;
        }

        data = 0;
    } else {
        LOGERROR << "unknown messsage type - " << data->msg_type() << std::endl;
    }

    if (data) data->release();

    return 0;
}

bool
Task::deliverControlMessage(ACE_Message_Block* data, ACE_Time_Value* timeout)
{
    static Logger::ProcLog log("deliverControlMessage", Log());
    LOGTIN << taskName_ << ' ' << data << std::endl;
    return processControlMessage(data);
}

bool
Task::processMessage(ACE_Message_Block* data)
{
    static Logger::ProcLog log("processMessage", Log());
    LOGINFO << taskName_ << ' ' << data << std::endl;

    // Dispatch on the message type: control or data
    //
    int type = data->msg_type();
    LOGDEBUG << "msg_type: " << type << std::endl;

    bool ok = false;
    if (type == MessageManager::kMetaData) {
        ok = processDataMessage(data);
    } else if (MessageManager::IsControlMessage(data)) {
        ok = processControlMessage(data);
    } else {
        LOGFATAL << "unknown type: " << data->msg_type() << std::endl;
        setError("Unknown message type");
        ok = false;
    }

    return ok;
}

bool
Task::processDataMessage(ACE_Message_Block* data)
{
    static Logger::ProcLog log("processDataMessage", Log());
    LOGINFO << taskName_ << ' ' << data << std::endl;
    Utils::Exception ex("IO::Task does not support input processing");
    log.thrower(ex);
    return true;
}

bool
Task::doTimeout()
{
    static Logger::ProcLog log("doTimeout", Log());
    LOGINFO << taskName_ << std::endl;
    Utils::Exception ex("IO::Task does not support alarming");
    log.thrower(ex);
    return true;
}

bool
Task::processControlMessage(ACE_Message_Block* data)
{
    static Logger::ProcLog log("processControlMessage", Log());
    MessageManager mgr(data->duplicate());
    LOGTIN << taskName_ << " control message: " << mgr.getControlMessageType() << std::endl;

    // Dispatch on control message type
    //
    bool ok = false;
    switch (mgr.getControlMessageType()) {
    case ControlMessage::kParametersChange:
        ok = doParametersChange(mgr.getControlMessage<ParametersChangeRequest>());
        break;

    case ControlMessage::kProcessingStateChange:
        ok = doProcessingStateChange(mgr.getControlMessage<ProcessingStateChangeRequest>());
        break;

    case ControlMessage::kRecordingStateChange:
        ok = doRecordingStateChange(mgr.getControlMessage<RecordingStateChangeRequest>());
        break;

    case ControlMessage::kShutdown: ok = doShutdownRequest(); break;

    case ControlMessage::kClearStats: ok = doClearStatsRequest(); break;

    case ControlMessage::kTimeout: ok = doTimeout(); break;

    default:
        LOGERROR << "unknown control message type - " << mgr.getControlMessageType() << std::endl;
        setError("Unknown control message type");
        break;
    }

    if (ok) {
        // Obey the SideCar programming contract for message passing. If we return success, we own the incoming
        // message.
        //
        data->release();
    }

    return ok;
}

bool
Task::doParametersChange(const IO::ParametersChangeRequest& msg)
{
    static Logger::ProcLog log("doParametersChange", Log());
    LOGINFO << std::endl;

    XmlRpc::XmlRpcValue params(msg.getValue());
    for (int index = 0; index < params.size();) {
        // Fetch the name of the parameter to change, then the XML-RPC value to apply.
        //
        std::string name(params[index++]);
        LOGDEBUG << "name: " << name << std::endl;
        if (index == params.size()) {
            LOGERROR << "missing value for parameter '" << name << "'" << std::endl;
            return true;
        }

        XmlRpc::XmlRpcValue value = params[index++];

        // Locate the parameter with the given name
        //
        ParameterMap::const_iterator pos = parameterMap_.find(name);
        if (pos != parameterMap_.end()) {
            try {
                if (msg.hasOriginalValues()) {
                    parameterVector_[pos->second]->setXMLOriginal(value);
                    LOGWARNING << taskName_ << " changed parameter " << name << " to " << value << std::endl;
                } else {
                    parameterVector_[pos->second]->setXMLValue(value);
                    LOGWARNING << taskName_ << ' ' << name << " = " << value << std::endl;
                }
            } catch (Parameter::InvalidValue& err) {
                LOGERROR << err.what() << std::endl;
                setError(err.what());
                return true;
            }
        } else {
            LOGERROR << "Unknown parameter name '" << name << "'" << std::endl;
        }
    }

    updateUsingDataValue();

    return true;
}

bool
Task::doRecordingStateChange(const RecordingStateChangeRequest& msg)
{
    Logger::ProcLog log("doRecordingStateChange", Log());
    LOGINFO << taskName_ << " - IGNORED" << std::endl;
    return true;
}

bool
Task::doProcessingStateChange(const ProcessingStateChangeRequest& msg)
{
    static Logger::ProcLog log("doProcessingStateChange", Log());
    LOGINFO << taskName_ << " value: " << msg.getValue() << std::endl;
    return enterProcessingState(msg.getValue());
}

bool
Task::doShutdownRequest()
{
    static Logger::ProcLog log("doShutdownRequest", Log());
    LOGINFO << taskName_ << " - IGNORED" << std::endl;
    return true;
}

bool
Task::doClearStatsRequest()
{
    static Logger::ProcLog log("doClearStatsRequest", Log());
    LOGINFO << taskName_ << std::endl;
    resetProcessedStats();
    return true;
}

bool
Task::send(const Messages::Header::Ref& msg, size_t channelIndex)
{
    IO::MessageManager manager(msg);
    return sendManaged(manager, channelIndex);
}

bool
Task::sendManaged(MessageManager& manager, size_t channelIndex)
{
    bool ok = true;

    // If there is nothing after us, just eat the message.
    //
    if (!next()) return true;

    // If the channel is a valid index, then have its recipient list handle delivery.
    //
    if (channelIndex < outputs_.size()) {
        ok = outputs_.getChannel(channelIndex).deliver(manager.getMessage());
    }

    // !!! Hack to keep some old unit tests running. Since Runner validates task connections, this should never
    // happen from well-formed XML configurations.
    //
    else if (channelIndex == 0) {
        ACE_Message_Block* data = manager.getMessage();
        if (data) {
            if (put_next(data, 0) == -1) {
                data->release();
                ok = false;
            }
        }
    }

    return ok;
}

void
Task::fillStatus(StatusBase& status)
{
    static Logger::ProcLog log("fillStatus", Log());

    size_t pendingQueueCount = msg_queue()->message_count();
    if (pendingQueueCount) { LOGDEBUG << "input queue size: " << pendingQueueCount << std::endl; }

    status.setSlot(TaskStatus::kProcessingState, processingState_);
    status.setSlot(TaskStatus::kError, error_);
    status.setSlot(TaskStatus::kConnectionInfo, connectionInfo_);
    status.setSlot(TaskStatus::kPendingQueueCount, int(pendingQueueCount));

    // Only allow parameter editing if there are more than two (there is always the editingEnabled_ parameter,
    // but users cannot change it via XML-RPC), and there is the processingState parameter, but we only allow
    // editing of it for algorithms
    //
    status.setSlot(TaskStatus::kHasParameters, parameterVector_.size() > taskParameterCount_);
    status.setSlot(TaskStatus::kUsingData, usingData_);

    // Calculate message counts and rates from the Stats object associated with each input.
    //
    size_t messageCount = 0;
    size_t byteRate = 0;
    size_t messageRate = 0;
    size_t dropCount = 0;
    size_t dupeCount = 0;
    size_t inputCount = inputStats_.size();
    for (size_t index = 0; index < inputCount; ++index) {
        Stats& s(inputStats_[index]);
        s.calculateRates();
        messageCount += s.getMessageCount();
        byteRate += s.getByteRate();
        messageRate += s.getMessageRate();
        dropCount += s.getDropCount();
        dupeCount += s.getDupeCount();
    }

    status.setSlot(TaskStatus::kMessageCount, int(messageCount / inputCount));
    status.setSlot(TaskStatus::kByteRate, int(byteRate / inputCount));
    status.setSlot(TaskStatus::kMessageRate, int(messageRate / inputCount));
    status.setSlot(TaskStatus::kDropCount, int(dropCount / inputCount));
    status.setSlot(TaskStatus::kDupeCount, int(dupeCount / inputCount));
}

namespace {
struct AddEditable {
    XmlRpc::XmlRpcValue& container;
    AddEditable(XmlRpc::XmlRpcValue& c) : container(c) {}
    void operator()(const Parameter::Ref& p)
    {
        if (p->isEditable()) {
            XmlRpc::XmlRpcValue param;
            p->describe(param);
            container.push_back(param);
        }
    }
};
} // namespace

void
Task::getCurrentParameters(XmlRpc::XmlRpcValue& value) const
{
    // Make sure we write into an XML-RCP array.
    //
    value.setSize(0);

    // Only add parameters that are editable by the user.
    //
    std::for_each(parameterVector_.begin(), parameterVector_.end(), AddEditable(value));
}

bool
Task::hasChangedParameters() const
{
    // Locate the first parameter that does not contain all original values.
    //
    return std::find_if(parameterVector_.begin(), parameterVector_.end(),
                        boost::bind(&Parameter::ValueBase::isNotOriginal, _1)) != parameterVector_.end();
}

namespace {
struct AddChanged {
    XmlRpc::XmlRpcValue& container;
    AddChanged(XmlRpc::XmlRpcValue& c) : container(c) {}
    void operator()(const Parameter::Ref& p)
    {
        if (p->isNotOriginal()) {
            XmlRpc::XmlRpcValue param;
            p->describe(param);
            container.push_back(param);
        }
    }
};
} // namespace

void
Task::getChangedParameters(XmlRpc::XmlRpcValue& value) const
{
    // Make sure we write into an XML-RCP array.
    //
    value.setSize(0);

    // Only add parameters that have been changed from their configured state.
    //
    std::for_each(parameterVector_.begin(), parameterVector_.end(), AddChanged(value));
}

void
Task::processingStateParameterChanged(const ProcessingStateParameter& value)
{
    Logger::ProcLog log("processingStateParameterChanged", Log());
    LOGINFO << value.getValue() << std::endl;

    // Only process a state change if it is valid and not the current processing state.
    //
    if (value.getValue() != ProcessingStateParameter::None() && value.getValue() != processingState_) {
        enterProcessingState(value.getValue());
    }
}

void
Task::resetProcessedStats()
{
    for (size_t index = 0; index < inputStats_.size(); ++index) { inputStats_[index].resetAll(); }
}

void
Task::addInputChannel(const Channel& channel)
{
    inputs_.add(channel);
    while (inputStats_.size() < inputs_.size()) inputStats_.push_back(Stats());
    channel.updateSenderUsingData(usingData_);
}

void
Task::updateInputStats(size_t channelIndex, size_t size, uint32_t sequenceCounter)
{
    inputStats_[channelIndex].updateInputCounters(size, sequenceCounter);
}

bool
Task::calculateUsingDataValue() const
{
    return outputs_.areAnyRecipientsUsingData() || alwaysUsingData_->getValue() ||
           processingState_ == ProcessingState::kAutoDiagnostic;
}

void
Task::setUsingData(bool value)
{
    Logger::ProcLog log("setUsingData", Log());
    LOGINFO << taskName_ << " current: " << usingData_ << " new: " << value << std::endl;

    if (!value) value = calculateUsingDataValue();

    // When transitioning valve states, reset the message counters.
    //
    if (value != usingData_) {
        usingData_ = value;
        if (value) resetProcessedStats();

        // Notify our input senders that our data-pulling state has changed.
        //
        inputs_.updateSendersUsingData(value);
    }
}

bool
Task::activateThreads(int count)
{
    Logger::ProcLog log("activateThreads", Log());
    LOGINFO << "count: " << std::endl;

    std::vector<ACE_thread_t> tids(count, ACE_thread_t());
    int rc = activate(threadParams_.flags,
                      count, // n_threads
                      0,     // force_active
                      threadParams_.priority,
                      -1, // grp_id
                      0,  // ACE_Task_Base*
                      0,  // ACE_hthread_t
                      0,  // void* stack
                      0,  // stack_size
                      &tids[0],
                      0 // thr_name
    );

    if (rc == -1) return false;

    if (threadParams_.cpuAffinity != -1) {
        for (int index = 0; index < count; ++index) {
            ACE_thread_t tid = tids[index];
            LOGINFO << "tid: " << tid << std::endl;
        }
    }

    return true;
}
