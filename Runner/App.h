#ifndef SIDECAR_RUNNER_APP_H // -*- C++ -*-
#define SIDECAR_RUNNER_APP_H

#include "ace/Message_Block.h"
#include <vector>

#include "boost/shared_ptr.hpp"

#include "Configuration/Loader.h"
#include "IO/Stream.h"

#include "Utils/CmdLineArgs.h"

class QFile;

namespace Logger {
class ConfiguratorFile;
class Log;
} // namespace Logger
namespace XmlRpc {
class XmlRpcValue;
}

namespace SideCar {
namespace IO {
class StatusBase;
}
namespace Runner {

class LogCollector;
class RemoteController;
class StatusEmitter;

/** Manager for one or more ACE stream objects, each of which contain one or more SideCar IO::Task objects. A
    runner application receives remote commands via XML-RPC requests, handled by its RemoteController object.
    Periodically, the runner emits status about all of its streams and stream tasks. This emission is managed by
    its StatusEmitter object.

    The runner application accepts the following options on the command-line:

    \code
    runner [-?|--help] [-d|--debug] [-H|--hostname HOST] [-i|--invalid] [-L|--logger LOG] [-M|--mcast ADDR] [-t|--tag
   TAG] [CONFIG] \endcode

    - \c d | \c debug enable verbose (debug level) log messages for ALL
    Logger::Log devices in the SideCar framework.

    - \c H | \c hostname set the host name to use when processing the XML
    configuration file, overriding the host name of the machine.

    - \c i | \c invalid start up the runner and all of its IO::Task objects in
    the IO::ProcessingState::kInvalid state. Without this option, the tasks
    will start up in the IO::ProcessingState::kRun state.

    - \c L | \c logger set a Logger configuration file to load and configure the
    all Logger::Log devices.

    - \c M | \c mcast assign a default IP address to use for multicast services.
    Note that if the XML configuration file defines a multicast address in
    the <host> tag used by the runner, then that value will be used instead
    of this one.

    - \c t | \c tag use the <host> stanza in the XML configuration file that
    has the corresponding tag value. This allows multiple runner processes to
    exist on the same host using the same XML configuration file.

    If the runner command has no CONFIG argument, it will read from standard
    input for the configuration data. This would be useful when the XML
    configuration is dynamically generated, and not in a file on the system.

    The App objects respond to the following XML-RCP requests:

    - getParameters -- obtain the runtime parameters of a particular IO::Task
    object of a specific stream. Requires two parameters in the request:
    stream index, and task index. Returns an XML-RPC array with status for
    each stream.

    - recordingChange -- start/stop data recording for all
    Algorithm::Controller objects enabled to record. Takes one parameter, a
    string that is the path of the directory into which Algorithm::Controller
    objects create their recordings. If the string is empty, recording stops.

    - setParameters -- change one or more runtime parameter values of a
    specific IO::Task. Takes one parameter, an XML-RPC array with two items
    for each parameter being changed: the parameter name, and the new
    parameter value.

    - shutdown -- command the runner to shutdown its streams. Takes no
    parameters.

    - stateChange -- command all IO::Task objects to enter a new processing
    state. Takes one parameter, an integer value that corresponds to one of
    the enumeration values in IO::ProcessingState::Value.

    NOTE: all of the above messages will run in a separate thread. The main
    thread of the App object, is essentially blocked at an
    ACE_Reactor::run_reactor_event_loop() call, and does not call back into any
    other App methods while the ACE Reactor is running.
*/

class App {
public:
    /** Obtain the log device for App instances

        \return Log device
    */
    static Logger::Log& Log();

    /** Get sole App instace.

        \return App pointer
    */
    static App* GetApp();

    /** Constructor for all App objects

        \param argc number of values in argv

        \param argv command line arguments (including executable name)
    */
    App(int argc, char* const* argv);

    /** Destructor. Removes the custom LogCollector from the Log service.
     */
    ~App();

    void initialize();

    /** Obtain the service name the runner has published.

        \return
    */
    const std::string& getServiceName();

    /** Obtain the set of all of the changed parameters for the runner.

        \param value container to hold the results
    */
    void getChangedParameters(XmlRpc::XmlRpcValue& value) const;

    /** Fill an XML-RPC variable with a description of the runtime parameter settings of an IO::Task object.

        \param streamIndex index of the stream containing the task

        \param taskIndex index of the task within the stream

        \param result storage for the result

        \return true if successful, false otherwise
    */
    bool getParameters(int streamIndex, int taskIndex, XmlRpc::XmlRpcValue& result) const;

    /** Change one or more runtime parameter settings of an IO::Task object.

        \param streamIndex index of the stream containing the task

        \param taskIndex index of the task within the stream

        \param params XML-RPC array containing parameter names and new values.

        \return true if successful, false otherwise
    */
    bool setParameters(int streamIndex, int taskIndex, const XmlRpc::XmlRpcValue& params) const;

    /** Fill in a RunnerStatus object with the current status of all managed IO::Stream objects.

        \param status object to update
    */
    void fillStatus(XmlRpc::XmlRpcValue& status);

    /** Start the RemoteController and StatusEmitter objects, and enter the ACE Reactor run loop, allowing data
        to flow.
    */
    void run();

    /** Stop the StatusEmitter object and the ACE Reactor run loop. Note that this method runs in a separate
        thread. Invokes ACE_Reactor::end_event_loop() which will cause the run() method to return from
        ACE_Reactor::run_reactor_event_loop().
    */
    void shutdown();

    /** Command all of the tasks to reset their input statistics.
     */
    void clearStats();

    /** Post a control message to all managed streams.

        \param data raw representation of an IO::ControlMessage object
    */
    void postControlMessage(ACE_Message_Block* data);

    /** Command all of the tasks to change their recording state if they are enabled for recording. If the given
        path is not empty, then it is a valid directory into which tasks shall write their recordings. If it is
        empty, then any previous recording activity stops.

        \param path full path of directory to use for the recording
    */
    void recordingStateChange(const std::string& path);

    /** Obtain the number of processing streams allocated in the runner

        \return stream count
    */
    size_t getStreamCount() const { return streams_.size(); }

    /** Change the runner's service name. Called when there is a naming conflict with another runner.

        \param name new name to use
    */
    void setServiceName(const std::string& name);

private:
    void initializeRealTime(const QString& scheduler);

    Utils::CmdLineArgs cla_;
    boost::shared_ptr<LogCollector> logCollector_;
    std::unique_ptr<Logger::ConfiguratorFile> loggerConfig_;
    std::vector<IO::Stream::Ref> streams_;
    std::unique_ptr<RemoteController> remoteController_;
    boost::shared_ptr<StatusEmitter> statusEmitter_;
    Configuration::Loader loader_;
    std::unique_ptr<Configuration::RunnerConfig> runnerConfig_;
#ifdef linux
    std::string statmPath_;
#endif
    static App* app_;
};

} // end namespace Runner
} // end namespace SideCar

/** \file
 */

#endif
