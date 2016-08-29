#ifndef SIDECAR_IO_TASK_H	// -*- C++ -*-
#define SIDECAR_IO_TASK_H

#include <map>
#include <string>
#include <vector>

#include "ace/svc_export.h"
#include "ace/Task.h"
#include "boost/shared_ptr.hpp"

#include "IO/Channel.h"
#include "IO/ProcessingState.h"
#include "IO/Stats.h"
#include "IO/TaskStatus.h"
#include "Messages/Header.h"

#include "Parameter/Parameter.h"

namespace Logger { class Log; }
namespace XmlRpc { class XmlRpcValue; }

namespace SideCar {
namespace IO {

class ControlMessage;
class MessageManager;
class ParametersChangeRequest;
class ProcessingStateChangeRequest;
class RecordingStateChangeRequest;
class StopProcessingRequest;
class Stream;

/** Derivation of ACE_Task for SideCar processing modules. An ACE_Task object may belong to an ACE_Module for
    processing in an ACE_Stream. Also, the ACE_Task class supports threaded processing via its svc() and
    activate() methods. Additional information on the ACE_Task, ACE_Module, and ACE_Stream components exist in
    the books``C++ Network Programming: Mastering Complexity Using ACE and Patterns'', and ``C++ Network
    Programming: Systematic Reuse with ACE and Frameworks''.

    A Task object is always in one of the following processing states:

    - kInitialize -- task is initialized
    - kAutoDiagnostic -- task is performming an auto-diagnostic check
    - kCalibrate -- task is performing a calibration
    - kRun -- task is in normal run mode
    - kStop -- task is not processing
    - kFailure -- task has encountered an unrecoverable/fatal error

    Transitions among the above states occur by calling enterProcessingState() with the new state. Enter a new
    state invokes a virtual method associated with the state; for instance, kCalibrate will run the
    calibrateStateProcessor(). If any processor method returns false, the task immediately transitions into the
    kFailure state, where it will remain until directed to enter the kInitialize state.

    The Task object keeps track of statistics in the form of message and byte counts processed by the task.
    These values are reset whenever the Task enters the kAutoDiagnostic, kCalibrate, and kRun. States.

    <h2>Connections</h2>

    SideCar tasks support directed connections among other tasks. Each task may have zero or more input
    connections from other other tasks as well as zero or more output connections to other tasks.

    \note Due to the way messages are delivered by following connections, cycles must be avoided
 
    <h2>On-Demand Processing</h2>

    A Task object only receives data when its isUsingData() method returns true (the value of the usingData_
    attribute). This attribute may be changed by others through the setUsingData() method, but the value given
    to this method may be amended by the return of calculateUsingDataValue().

    Normally, a Task will return true from isUsingData() when one of the following is true:

    - any of its recipients isUsingData() returns true
    - the alwaysUsingData_ parameter is enabled
    - the current processing state is ProcessingState::kAutoDiagnostic

    Derived classes may override calculateUsingDataValue() in order to amend the above conditions.

    The setUsingData() of Task will invoke calculateUsingDataValue() only if the value given to it is false.
    This supports the case where a downstream input task turns off for some reason, but there are other output
    tasks asking for data (or other conditions listed above are met). Due to this linkage, care must be
    exercised when overriding the setUsingData() or calculateUsingDataValue() methods: it is fragile and easy to
    break.

    \note The on-demand processing logic described above operates in a multi-threaded environment. Currently,
    changes to the boolean usingData_ attribute occur without mutex protection which may be bad mojo.
*/
class Task : public ACE_Task<ACE_MT_SYNCH>
{
    using Super = ACE_Task<ACE_MT_SYNCH>;
public:

    /** Default flags to use for processing threads. See ace/Task_T.h
     */
    enum {
	kDefaultThreadFlags = THR_NEW_LWP | THR_JOINABLE | THR_SCHED_DEFAULT | THR_INHERIT_SCHED
    };

    /** Parameters used by the activateThreads() method that control how threads operate. Currently supported
        values:

	- ACE thread flags
	- Thread priority
	- CPU affinity (linux only)

	Custom thread parameters may be installed in a Task object via the Task::setThreadParams() method.
    */
    struct ThreadParams {
	ThreadParams() : flags(kDefaultThreadFlags), priority(ACE_DEFAULT_THREAD_PRIORITY), cpuAffinity(-1) {}
	long flags;
	long priority;
	long cpuAffinity;
    };

    /** Shared reference counter for Task objects.
     */
    using Ref = boost::shared_ptr<Task>;

    /** Container of Parameter object references.
     */
    using ParameterMap = std::map<std::string,size_t>;
    using ParameterVector = std::vector<Parameter::Ref>;

    /** Obtain the log device for objects of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor. Since this is an abstract base class, only derived classes may be instantiated.
     */
    Task(bool usingData = false);

    /** Destructor. Here only to emit a logging message.
     */
    ~Task();

    /** Obtain the name of the task.

        \return task name
    */
    const std::string& getTaskName() const { return taskName_; }

    /** Set the task's internal name. This is used in log messages, and as the value for the StatusBase::kName
        field.

        \param taskName name to use.
    */
    void setTaskName(const std::string& taskName) { taskName_ = taskName; }

    /** Obtain the stream to which this task belongs

        \return Stream pointer or NULL if not part of a stream.
    */
    boost::weak_ptr<Stream> getStream() const { return stream_; }

    /** Set the Stream object in which this Task object resides. May be NULL>

        \param stream the Stream object to associate with
    */
    void setStream(const boost::shared_ptr<Stream>& stream) { stream_ = stream; }

    /** Place a given message into this Tasks's input processing queue. Identify it as coming from a given
        channel.

	\param msg the input message to use

	\param channel the input channel to use

        \return true if successful
    */
    bool putInChannel(const Messages::Header::Ref& msg, size_t channel);

    /** Override of ACE_Task method. Handles delivery of incoming messages for the task. Invokes
        deliverControlMessage() for control messages, and if the task is the next recipient, it invokes
        deliverDataMessage() for data messages.
        
        \param data message data

        \param timeout amount of time to try to deliver the message

        \return 0 if successful, -1 otherwise
    */
    int put(ACE_Message_Block* data, ACE_Time_Value* timeout = 0) override;

    /** Record an error message. The error will remain held until clearError() is called.
        
        \param text the text describing the error

        \param force if true, overwrite any existing error message
    */
    void setError(const std::string& text, bool force = false);

    /** Fetch the current error text
        
        \return error text
    */
    const std::string& getError() const { return error_; }

    /** Clear the held error text.
     */
    void clearError() { error_ = ""; }

    /** Determine if the task has an error posted.
        
        \return true if so
    */
    bool hasError() const { return ! error_.empty(); }

    /** Register a configurable parameter with the controller. Only invoked by the algorithm DLL.
        
	\param parameter the parameter value to register

	\return true if registered, false if name already registered
    */
    bool registerParameter(const Parameter::Ref& property);

    /** Unregister a configurable parameter with the controller. Only invoked by the algorithm DLL.
        
	\param parameter the parameter value to unregister

	\return true if unregistered, false if not found
    */
    bool unregisterParameter(const Parameter::Ref& property);
    
    /** Determine if this task has any parameter objects that have values that differ from their startup
        configuration.
        
        \return true if so
    */
    bool hasChangedParameters() const;

    /** Obtain the current processing state indicator.

        \return current state
    */
    ProcessingState::Value getProcessingState() const { return processingState_; }

    /** Determine if the task is in a message processing state. Currently, there are three processing states:

	1) kAutoDiagnostic
	2) kCalibrate
	3) kRun

        \return true if so
    */
    bool isActive() const { return ProcessingState::IsActive(getProcessingState()); }

    /** Determine if the task is in the calibration state

        \return true if so
    */
    bool isInCalibrationState() const { return ProcessingState::kCalibrate == getProcessingState(); }

    /** Determine if the task is in the run state

        \return true if so
    */
    bool isInRunState() const { return ProcessingState::kRun == getProcessingState(); }

    /** Determine if the task is in the stop state

        \return true if so
    */
    bool isInStopState() const { return ProcessingState::kStop == getProcessingState(); }

    /** Obtain a read-only reference to the message Stats object for this Task object.

        \return Stats reference
    */
    const Stats& getInputStats(size_t channelIndex) const { return inputStats_[channelIndex]; }

    /** Update internal processing statistics using values from the latest message received by the task.

        \param byteCount number of bytes received

        \param sequenceCounter sequence counter of the received message
    */
    void updateInputStats(size_t channelIndex, size_t byteCount, uint32_t sequenceCounter);

    /** Set the unique ID for this task. Used when checking whether it is a

	recipient of a message from another task.
    */
    void setTaskIndex(size_t taskIndex) { taskIndex_ = taskIndex; }

    /** Obtain the unique ID for this task.

        \return task ID
    */
    size_t getTaskIndex() const { return taskIndex_; }

    /** Add an input channel to the task.

        \param channel the channel to add
    */
    void addInputChannel(const Channel& channel);

    /** Add an input channel to the task.

        \param channel the channel to add
    */
    void addOutputChannel(const Channel& channel) { outputs_.add(channel); }

    /** Obtain an existing input channel.
        
        \param index the position of the channel to fetch

        \return read-only Channel reference
    */
    const ChannelVector& getInputChannels() const { return inputs_; }

    /** Obtain the number of input channels defined for this task.
        
        \return number of input channels
    */
    size_t getNumInputChannels() const { return inputs_.size(); }
    
    /** Obtain a specific input channel defined for this task
        
        \param index which input channel to fetch

        \return read-only reference to the Channel
    */
    const Channel& getInputChannel(size_t index) { return inputs_.getChannel(index); }

    /** Obtain the index of an input channel with a given name
        
        \param name the name to look for

        \return index of found input channel, or -1 if none found
    */
    size_t getInputChannelIndex(const std::string& name) { return inputs_.findName(name); }

    /** Obtain an existing output channel.
        
        \param index the position of the channel to fetch

        \return read-only Channel reference
    */
    const ChannelVector& getOutputChannels() const { return outputs_; }

    /** Obtain the number of output channels defined for this task.
        
        \return number of output channels
    */
    size_t getNumOutputChannels() const { return outputs_.size(); }

    /** Obtain a specific output channel defined for this task
        
        \param index which output channel to fetch

        \return read-only reference to the Channel
    */
    const Channel& getOutputChannel(size_t index) { return outputs_.getChannel(index); }

    /** Obtain the index of an output channel with a given name
        
        \param name the name to look for

        \return index of found output channel, or -1 if none found
    */
    size_t getOutputChannelIndex(const std::string& name) { return outputs_.findName(name); }

    /** Obtain an XML-RPC definition of the current runtime parameter values.
        
        \param value container to hold the definitions
    */
    void getCurrentParameters(XmlRpc::XmlRpcValue& value) const;

    /** Obtain an XML-RPC definition of all changed runtime parameter values, those with values different from
        that found in the configuration file or at initialization time.

        \param value container to hold the definitions
    */
    void getChangedParameters(XmlRpc::XmlRpcValue& value) const;

    /** Obtain the status class name for this class. Derived classes should override to return a value unique to
        the class.
        
        \return NULL-terminated C string
    */
    virtual const char* getStatusClassName() const { return TaskStatus::GetClassName(); }

    /** Obtain the number of slots in the TaskStatus XML status object.
        
        \return slot count
    */
    virtual size_t getStatusSize() const { return TaskStatus::kNumSlots; }

    /** Obtain status information from the task. Fills in message processing stats.
        
        \param status status object to fill in
    */
    virtual void fillStatus(StatusBase& status);

    /** Obtain the current usingData_ setting.

        \return true if Task uses data
    */
    bool isUsingData() const { return usingData_; }

    /** Update this object's usingData_ value.
     */
    void updateUsingDataValue() { setUsingData(false); }

    /** Set a new usingData_ value. If it is false, the value may be changed due to a true value returned by
        calculateUsingDataValue(). If the new state is different from then current usingData_ value, this
        routine will invoke resetProcessedStats() and notify upstream connections of the status change.

	\note Exercise care if changing this or overriding it in a derived class. A more appropriate override
        may be the calculateUsingDataValue() methd if the derived class has additional conditions to determine
        whether data should flow. At the least, derived methods must invoke this one.

	\param value new value to use
    */
    virtual void setUsingData(bool value);

    /** Obtain a desired usingData_ value depending on othe Task settings, such as recipient usingData_ values,
        the alwaysUsingData_ parameter, and the current processing state.

	\note Exercise care if changing this or overriding it in a derived class. Derived classes should usually
        invoke this method in a boolean clause such as:

	\code
	return return (foobar_ == 3) || Super::calculateUsingDataValue();
	\endcode

        \return true if using data
    */
    virtual bool calculateUsingDataValue() const;

    /** Set the thread parameters that the task will use in its activateThreads() call. The task records the
        thread parameters since initialization and configuration may not immediately invoke activateThreads().

        \param threadParams the new thread parameters to use.
    */
    void setThreadParams(const ThreadParams& threadParams) { threadParams_ = threadParams; }

    /** Obtain the current thread parameter settings.
        
        \return 
    */
    const ThreadParams& getThreadParams() const { return threadParams_; }

    /** Spawn count threads that will each run in the task's svc() method, which derived classes must define
        (see ACE_Task::svc()).

        \param count the number of threads to launch

        \return true if successful, false otherwise
    */
    bool activateThreads(int count);

protected:

    /** Change the tasks processing state indicator. Invokes the appropriate processor method for the new state,
	iff the transition from the current state to the new one is valid.

	\param state new state to enter

	\return true if successful, false if entered kFailure state
    */
    bool enterProcessingState(ProcessingState::Value state);

    /** Change to the last non-failure processing state.
        
        \return true if successful
    */
    bool enterLastProcessingState();

    /** Processor for the transition into the initialize state. This implementation does nothing.
        
	\return true if successful.
    */
    virtual bool enterInitializeState();

    /** Processor for the transition into the auto-diagnostic state. This implementation simply resets the
	processing statistics.
        
	\return true if successful.
    */
    virtual bool enterAutoDiagnosticsState();

    /** Processor for the transition into the calibrate state. This implementation simply resets the processing
	statistics.
        
	\return true if successful.
    */
    virtual bool enterCalibrateState();

    /** Processor for the transition into the run state. This implementation simply resets the processing
        statistics.
        
	\return true if successful.
    */
    virtual bool enterRunState();

    /** Processor for the transition into the stop state. This implementation does nothing.
        
	\return true if successful.
    */
    virtual bool enterStopState();

    /** Processor for the transition into the failure state. This implementation does nothing.
        
	\return true if successful.
    */
    virtual bool enterFailureState();

    /** Reset the processed counters.
     */
    void resetProcessedStats();

    /** Give a control message to the task. This implementation just invokes processControlMessage().
        
        \param data message to deliver

        \param timeout amount of time to spend trying to deliver message

        \return true if successful
    */
    virtual bool deliverControlMessage(ACE_Message_Block* data, ACE_Time_Value* timeout);

    /** Give a message to the task to do with as it pleases. Derived classes must define.

        \param data message to deliver

        \param timeout amount of time to spend trying to deliver message

        \return true if successful
    */
    virtual bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout) = 0;

    /** Process a message, data or control. This is a helper routine for derived classes that perform their
        message processing in a separate thread (it is not used by Task itself). Depending on the type of
        message held in the given \a data parameter, it invokes processDataMessage() or processControlMessage().
        
        \param data the message to process

        \return true if successful
    */
    bool processMessage(ACE_Message_Block* data);

    /** Process a data message. \warning WARNING: this implementation throws an exception if invoked.

        \param data data message

        \return true if successful
    */
    virtual bool processDataMessage(ACE_Message_Block* data);

    /** Process a timeout expired control message. \warning WARNING: this implementation throws an exception if
        invoked.
        
        \return true if successful 
    */ 
    virtual bool doTimeout();

    /** Process a control message. Dispatches on the control type to the appropriate 'do' method (eg.
        doProcessingStateChange()).
        
        \param data control message data

        \return true if successful
    */
    virtual bool processControlMessage(ACE_Message_Block* data);

    /** Process runtime parameter changes from an XML-RPC request.
        
        \param msg new parameter settings

        \return true if successful
    */
    virtual bool doParametersChange(const ParametersChangeRequest& request);

    /** Respond to a processing state change meessage. Calls enterProcessingState() to perform the state change.
        
        \param request new state to enter

        \return true if successful
    */
    virtual bool doProcessingStateChange(const ProcessingStateChangeRequest& request);

    /** Respond to a recording state change meessage. This implementation ignores such a request.

        \return true if successful
    */
    virtual bool doRecordingStateChange(const RecordingStateChangeRequest& request);

    /** Respond to a SHUTDOOWN control meessage. This implementation ignores such a request, but derived classes
        may override to handle the situation the application shuts down.

	\note This is different than entering the kStop processing state -- a SHUTDOWN message signals a
        ShutdownMonitor task to halt ACE processing.

        \return true if successful
    */
    virtual bool doShutdownRequest();

    /** Respond to a ClearStatsRequest control messsage. Resets the running message tally to zeros.

        \return true if successful
    */
    virtual bool doClearStatsRequest();

    /** Set the status text that represents the connection information for the task. This value will be used
        when updating TaskStatus XML values.
        
        \param connectionInfo new value to assign
    */
    void setConnectionInfo(const std::string& connectionInfo) { connectionInfo_ = connectionInfo; }

    virtual bool send(const Messages::Header::Ref& msg, size_t channelIndex);

    bool sendManaged(MessageManager& mgr, size_t channelIndex);

private:

    /** Definition of the enum range for the processingStateParameter_ parameter.
     */
    struct ProcessingStateEnumTraits : public Parameter::Defs::EnumTypeTraitsBase
    {
	using ValueType = ProcessingState::Value;
	static ValueType GetMinValue() { return ProcessingState::kAutoDiagnostic; }
	static ValueType GetMaxValue() { return ProcessingState::kStop; }
	static const char* const* GetEnumNames();
    };

    using ProcessingStateEnumDef = Parameter::Defs::Enum<ProcessingStateEnumTraits>;
    using ProcessingStateParameter = Parameter::TValue<ProcessingStateEnumDef>;

    /** Notification handler invoked when the processing state value changes.

        \param value new value to use
    */
    void processingStateParameterChanged(const ProcessingStateParameter& value);

    boost::weak_ptr<Stream> stream_; ///< The IO::Stream object we are a part of
    std::string taskName_;	///< The name assigned to this task
    std::string error_;		///< The last error encountered by the task

    size_t taskIndex_;		///< The index of this task in its Stream
    size_t taskParameterCount_;	///< The number of parameters defined by Task

    ChannelVector inputs_;	///< Collection of channels defined for inputs
    std::vector<Stats> inputStats_;
    ChannelVector outputs_;	///< Collection of channels defined for outputs
    ParameterMap parameterMap_;	///< Registered runtime parameters
    ParameterVector parameterVector_;

    ProcessingStateParameter::Ref processingStateParameter_; ///< State param
    Parameter::BoolValue::Ref editingEnabled_;		     ///< Editable param
    std::string connectionInfo_;			     ///< ???
    ProcessingState::Value processingState_; ///< Current processing state set
    ProcessingState::Value lastProcessingState_; ///< Last processing state set
    Parameter::BoolValue::Ref alwaysUsingData_;
    ThreadParams threadParams_;
    bool usingData_;		///< True if the task uses data from above

};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
