#ifndef SIDECAR_ALGORITHMS_CONTROLLER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CONTROLLER_H

#include "ace/DLL.h"
#include <string>

#include "QtXml/QDomNode"

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"
#include "boost/timer.hpp"
#include "boost/weak_ptr.hpp"

#include "Algorithms/ControllerStatus.h"
#include "Algorithms/ProcessingStat.h"
#include "IO/Module.h"
#include "IO/ProcessingState.h"
#include "IO/Task.h"
#include "Logger/Priority.h"
#include "Messages/Header.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Algorithms {

class Algorithm;
class Recorder;

/** Controller for an Algorithm object found in a separate DLL. Isolates the algorithm from specifics regarding
    data source and destination. Since the Controller class derives from IO::Task, it can exist as part of an
    IO::Stream, which is usually how they appear in the SideCar system.

    The openAndInit() method provides a Controller with the name of the algorithm to load. This routine invokes
    loadAlgorithm() to handle the DLL loading in a platform-neutral way. If the load was successful, the
    Controller initializes the Algorithm object by invoking Algorithm::startup(). If that step succeeds, the
    Controller starts a separate thread (which runs the svc() method) to handle message passing to the
    algorithm's registered message processors.

    <h2>Algorithm Output Recording</h2>

    Each controller has its own Recorder object that manages the recording state for the Controller. Recordings
    contain values from the output channels defined for the Controller. Current, there is no way to isolate
    recording to just a subset of the output channels.

    Recording must be enabled for an Controller/Algorithm before it will allow its Recorder object to respond to
    Recorder::start() and Recorder::stop() messages. The Controller has a runtime parameter named
    "recordingEnabled" which contains the enabled state for recording. When set to \c true, the Controller will
    respond to IO::RecordingStateChangeRequest messages. The user can change this value through the
    SideCar::GUI::Master application's parameter editor interface.
*/
class Controller : public IO::Task {
    using Super = IO::Task;

public:
    /** Reference to self.
     */
    using Ref = boost::shared_ptr<Controller>;

    /** Obtain log device for Controller objects to use.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory method that creates a new Controller object. This is the only way to create a new Controller
        object. Returned objects have been initialized, but they do not have an Algorithm under their control;
        that occurs in the openAndInit() method.

        \return reference to new Controller object
    */
    static Ref Make();

    /** Destructor. Only present to emit log messages.
     */
    ~Controller();

    /** Open the controller. Attempts to load the DLL that contains the Algorithm class to instantiate. If
        loading was successful, invokes the Algorithm::startup() method to let the algorithm finish initializing
        itself. Finally, the controller spawns a new thread which will handle all message processing of the
        controller, including data writes/read writes to/from the Algorithm object.

        \param algorithmName name of the algorithm to load

        \param algorithm if not NULL, use as the Algorithm object to manage. Used by the unit test to supply a
        test algorithm. NOTE: if used, the controller does not attempt to load in the DLL with the name
        algorithmName. If you need the DLL loaded, you must do so yourself.

        \return true if successful
    */
    bool openAndInit(const std::string& algorithmName, const std::string& serviceName = "", Algorithm* algorithm = 0,
                     long threadFlags = kDefaultThreadFlags, long threadPriority = ACE_DEFAULT_THREAD_PRIORITY,
                     bool threaded = true);

    /** Shutdown the task. Override of ACE_Task method. This is the canonical way to stop an ACE task. The close
        method gets called in two distinct situations, indicated by the value of the the given \a flags
        argument:

        - \c non-zero the controller is being closed by an external entity such as an IO::Module that is hosting
          the controller.
        - \c zero a thread spawned by the controller in the openAndInit() method has returned from the svc()
          method.

        If \a flags is non-zero, the method deactivates the internal message queue to signal the service thread
        to stop processing. The method then waits for the thread to exit before it then invokes
        Algorithm::shutdown() to notify the algorithm that it is no longer running.

        \param flags indication of the situation in which the method is being called (see above)

        \return 0 if successful, -1 otherwise
    */
    int close(u_long flags = 0) override;

    /** Obtain the name of the algorithm DLL the controller will load and manage.

        \return algorithm name
    */
    const std::string& getAlgorithmName() const { return algorithmName_; }

    /** Fetch the loaded algorithm. NOTE: do not save this pointer since the algorithm object may be unloaded at
        any time, making the pointer invalid.

        \return pointer to loaded algorithm
    */
    Algorithm* getAlgorithm() const { return algorithm_.get(); }

    /** Post a control message to the internal processing queue.

        \param msg control message to post

        \return true if successful
    */
    bool injectControlMessage(const IO::ControlMessage& msg);

    /** Post a control message to change the run state

        \param msg control message to post

        \return true if successful
    */
    bool injectProcessingStateChange(IO::ProcessingState::Value state);

    /** Override of IO::Task method. Obtain the status class name for algorithm controller tasks.

        \return NULL-terminated C string
    */
    const char* getStatusClassName() const override { return ControllerStatus::GetClassName(); }

    /** Obtain the number of slots in a ControllerStatus XML array filled in by the algorithm. By default, this
        is ControllerStatus::kNumSlots. However, if an algorithm adds its own custom slots, the returned value
        will be greater.

        \return
    */
    size_t getStatusSize() const override;

    /** Override of IO::Task method. Obtain status information from the controller. Fills recording state, and
        info data from the Algorithm (if any)

        \param status status object to fill in
    */
    void fillStatus(IO::StatusBase& status) override;

    /** Manually update the held ProcessingStats attribute with the given sample value.

        \param delta processing duration to record
    */
    void addProcessingStatSample(const Time::TimeStamp& delta);

    /** Start a timer for tracking algorithm processing time. The timer stops when the managed algorithm emits a
        message by calling send().
    */
    void beginStatsProcessing() { processingStat_.beginProcessing(); }

    /** Override of IO::Task method. Determines whether the algorithm is currently using data.

        \return true if recording is enabled
    */
    bool calculateUsingDataValue() const override;

    /** Set whether the controller manages the ProcessingStats attribute. If false, the algorithm is responsible
        for posting processing duration values via addProcessingStatSample().

        \param state new state
    */
    void setStatsManaged(bool state) { statsManaged_ = state; }

    /** Install the QDomNode of the XML parsed configuration file for the algorithm.

        \param node the QDomNode that refers to an <algorithm> entity.
    */
    void setXMLDefinition(const QDomNode& node) { xmlConfiguration_ = node.cloneNode(); }

    const QDomNode& getXMLDefinition() const { return xmlConfiguration_; }

    /** Algorithms can use this method to set a periodic alarm every N seconds. When the alarm goes off, the
        controller will invoke the algorithm's processAlarm() method.

        \param timerSecs the number of seconds between processAlarm() calls.
        If set to zero, disable alarms.
    */
    void setTimerSecs(int timerSecs);

    /** Obtain the current alarm period. If zero, alarms are disabled.

        \return alarm period in seconds
    */
    int getTimerSecs() { return timerSecs_; }

private:
    /** Constructor. Initializes the object, but does not load an algorithm; that is done in the open() method.
     */
    Controller();

    /** Send a type-specific message to the processing stream. Used by the algorithm to place new messages onto
        the processing stream.

        \param manager contains the message to send

        \return true if successful, false otherwise
    */
    bool send(const Messages::Header::Ref& msg, size_t channelIndex) override;

    /** Load the DLL library containing the algorithm class to instantiate, and attempt to create a new
        algorithm object.
    */
    bool loadAlgorithm();

    /** Set a weak reference to ourselves so that we can give out shared_ptr refences to ourselves.

        \param self reference to initialize with
    */
    void setSelf(const Ref& self) { self_ = self; }

    /** Obtain a shared_ptr reference to ourselves. The template parameter indicates a derived class that is the
        actual type of 'this'.

        \return new shared_ptr reference
    */
    template <typename T>
    boost::shared_ptr<T> getSelf() const
    {
        return boost::dynamic_pointer_cast<T>(self_.lock());
    }

    /** Override of IO::Task method. Enter the initialized state. Invokes Algorithm::reset() method on the
        managed Algorithm object.

        \return true if successful
    */
    bool enterInitializeState() override;

    /** Override of IO::Task method. Enter the diagnostics state. Invokes Algorithm::beginAutoDiag() method on
        the managed Algorithm object.

        \return true if successful
    */
    bool enterAutoDiagnosticsState() override;

    /** Override of IO::Task method. Enter the calibrate state. Invokes Algorithm::beginCalibration() method on
        the managed Algorithm object.

        \return true if successful
    */
    bool enterCalibrateState() override;

    /** Override of IO::Task method. Enter the run state. Invokes Algorithm::beginRun() method on the managed
        Algorithm object.

        \return true if successful
    */
    bool enterRunState() override;

    /** Override of IO::Task method. Enter the stop state. Invokes Algorithm::stop() method on the managed
        Algorithm object.

        \return true if successful
    */
    bool enterStopState() override;

    /** Obtain a message from the input message queue and process it.

        \return true if successful, false otherwise
    */
    bool fetchAndProcessOneMessage();

    /** Process the given opaque message.

        \param data opaque message to process
    */
    void processOneMessage(ACE_Message_Block* data);

    /** Override of ACE Task method. Pulls messages from the internal queue and processes them. Forwards data
        messages to the managed algorithm. Runs in a separate thread. Only returns after the message queue is
        shutdown.

        \return 0 if successful, -1 otherwise
    */
    int svc() override;

    /** Implementation of Task method. Place message into processing queue, regardless of message type.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverDataMessage(ACE_Message_Block* data, ACE_Time_Value* timeout) override;

    /** Override of IO::Task method. Treat the same as a data message -- place on the svc thread's message
        queue.

        \param data raw data to send

        \param timeout amount of time to spend trying to do the send

        \return true if successful
    */
    bool deliverControlMessage(ACE_Message_Block* data, ACE_Time_Value* timeout) override;

    /** Override of IO::Task method. Forwards the data to the algorithm for processing. Updates processing
        stats.

        \param data message data to process

        \return true if successful
    */
    bool processDataMessage(ACE_Message_Block* data) override;

    /** Override of IO::Task method. Process recording state change from an XML-RPC request.

        \param msg new recording state

        \return true if successful
    */
    bool doRecordingStateChange(const IO::RecordingStateChangeRequest& msg) override;

    /** Override of IO::Task method. Invoke the algorithm's processAlarm() method.

        \return true if successful
    */
    bool doTimeout() override;

    /** Override of IO::Task method. Clear the algorithm processing statistics.

        \return true if successful, false otherwise
    */
    bool doClearStatsRequest() override;

    /** Override of IO::Task method. Call Algorithm::beginParameterChanges() before invoking
        Task::doParametersChange(), and call Algorithm::endParameterChanges() after.

        \param msg new parameter settings

        \return true if successful
    */
    bool doParametersChange(const IO::ParametersChangeRequest& request) override;

    /** Method invoked in a separate thread that provides a periodic timer to invoke an algorithm's
        processAlarm() method. A timer thread starts when setTimerSecs() is called with a positive value.
    */
    void alarmTimerProc();

    /** Begin writing to a file the outputs of the hosted algorithm. If the algorithm is configured to write to
        more than one channel, each channel will have its own recording file identified by its channel suffix
        append to the given file path name.

        \param path the path to the file to write to

        \return true if successful, false otherwise
    */
    bool startRecordings(const std::string& path);

    /** Stop recording algorithm output.

        \return true if successful, false otherwise
    */
    bool stopRecordings();

    // --- Attributes ---

    boost::weak_ptr<Controller> self_;       ///< Weak reference to ourselves
    std::string algorithmName_;              ///< Name of the algorithm (DLL) to control
    boost::scoped_ptr<Algorithm> algorithm_; ///< Loaded algorithm object
    std::vector<Recorder*> recorders_;       ///< Objects that handle recording

    /** Definition of the enum range for the logLevel_ parameter.
     */
    struct LogLevelEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
        using ValueType = Logger::Priority::Level;
        static ValueType GetMinValue() { return Logger::Priority::kNone; }
        static ValueType GetMaxValue() { return Logger::Priority::kDebug3; }
        static const char* const* GetEnumNames() { return Logger::Priority::GetLongNames(); }
    };

    using LogLevelParameter = Parameter::TValue<Parameter::Defs::Enum<LogLevelEnumTraits>>;

    /** Notification handler called when the logLevel_ parameter changes.

        \param parameter reference to parameter that changed
    */
    void logLevelChanged(const LogLevelParameter& parameter);

    /** Run-time parameter that controls the amount of log messages emitted by the algorithm.
     */
    LogLevelParameter::Ref logLevel_;

    /** Run-time parameter that determines if we will record data upon receipt of the
        IO::RecordingStateChangeRequest message.
    */
    Parameter::BoolValue::Ref recordingEnabled_;

    ProcessingStat processingStat_; ///< Algorithm processing statistics
    QDomNode xmlConfiguration_;     ///< XML configuration for this algorithm
    bool recording_;                ///< True if currently recording data
    bool statsManaged_;             ///< True if managing stats
    bool threaded_;                 ///< If true algorithm processing is in separate thread
    int timerSecs_;                 ///< The number of seconds between each doTimeout call
    boost::thread timerThread_;     ///< Thread that runs alarmTimerProc

    struct IncomingNotifier;
    friend class IncomingNotifier;
    friend class Algorithm;
};

using ControllerModule = IO::TModule<Controller>;

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
