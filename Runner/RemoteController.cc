#include "ace/Reactor.h"

#include "IO/ProcessingState.h"
#include "IO/ProcessingStateChangeRequest.h"
#include "IO/ShutdownRequest.h"

#include "Logger/Log.h"
#include "XMLRPC/XmlRpcServer.h"
#include "XMLRPC/XmlRpcServerMethod.h"
#include "XMLRPC/XmlRpcValue.h"

#include "App.h"
#include "RemoteController.h"

using namespace SideCar;
using namespace SideCar::Zeroconf;
using namespace SideCar::Runner;

struct AppMethodBase : public XmlRpc::XmlRpcServerMethod {
    App& app_;

    AppMethodBase(App& app, XmlRpc::XmlRpcServer* server, const char* name) :
        XmlRpc::XmlRpcServerMethod(name, server), app_(app)
    {
    }
};

/** App method to clear processing statistics for all streams.
 */
struct ClearStats : public AppMethodBase {
    /** Constructor

        \param app the App object to work with

        \param server the server that will execute the method
    */
    ClearStats(App& app, XmlRpc::XmlRpcServer* server) : AppMethodBase(app, server, "clearStats") {}

    /** Perform the XML-RPC request. Implementation of XmlRpcServerMethod interface.

        \param params ignored

        \param result ignored
    */
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result)
    {
        app_.clearStats();
        result = true;
    }
};

/** App method to fetch the runtime parameters for a particular IO::Task object.
 */
struct GetChangedParameters : public AppMethodBase {
    /** Constructor

        \param app the App object to work with

        \param server the server that will execute the method
    */
    GetChangedParameters(App& app, XmlRpc::XmlRpcServer* server) : AppMethodBase(app, server, "getChangedParameters") {}

    /** Perform the XML-RPC request. Implementation of XmlRpcServerMethod interface.

        \param params arguments to the request. Must be an array with two
        integer values, the stream index and task index to query.

        \param result XML-RPC array containing the description of the runtme
        parameter values.
    */
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) { app_.getChangedParameters(result); }
};

/** App method to Fetch the runtime parameters for a particular IO::Task object.
 */
struct GetParameters : public AppMethodBase {
    /** Constructor

        \param app the App object to work with

        \param server the server that will execute the method
    */
    GetParameters(App& app, XmlRpc::XmlRpcServer* server) : AppMethodBase(app, server, "getParameters") {}

    /** Perform the XML-RPC request. Implementation of XmlRpcServerMethod interface.

        \param params arguments to the request. Must be an array with two
        integer values, the stream index and task index to query.

        \param result XML-RPC array containing the description of the runtme
        parameter values.
    */
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result)
    {
        static Logger::ProcLog log("GetParameters::execute", RemoteController::Log());
        int streamIndex = params[0];
        int taskIndex = params[1];
        LOGINFO << "streamIndex: " << streamIndex << " taskIndex: " << taskIndex << std::endl;
        if (!app_.getParameters(streamIndex, taskIndex, result)) { result = "Invalid task / stream index"; }
    }
};

/** App method to change the recording state for all IO::Task objects.
 */
struct RecordingChange : public AppMethodBase {
    /** Constructor

        \param app the App object to work with

        \param server the server that will execute the method
    */
    RecordingChange(App& app, XmlRpc::XmlRpcServer* server) : AppMethodBase(app, server, "recordingChange") {}

    /** Perform the XML-RPC request. Implementation of XmlRpcServerMethod interface.

        \param params arguments to the request. Must be an array with one
        string value: the path of a directory to record into if starting, or an
        empty string if stopping.

        \param result unused
    */
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result)
    {
        std::string recordingPath = params[0];
        app_.recordingStateChange(recordingPath);
        result = true;
    }
};

/** App method to change one or more runtime parameters for a particular IO::Task object.
 */
struct SetParameters : public AppMethodBase {
    /** Constructor

        \param app the App object to work with

        \param server the server that will execute the method
    */
    SetParameters(App& app, XmlRpc::XmlRpcServer* server) : AppMethodBase(app, server, "setParameters") {}

    /** Perform the XML-RPC request. Implementation of XmlRpcServerMethod interface.

        \param params arguments to the request. Must be an array with three
        value values:
        - streamIndex -- the index of the stream to work with
        - taskIndex -- the index of the task to work with
        - changes -- an XML-RPC array containing two entries for each change,
        a parameter name followed by the new value.

        \param result unused
    */
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result)
    {
        static Logger::ProcLog log("SetParameters::execute", RemoteController::Log());
        int streamIndex = params[0];
        int taskIndex = params[1];
        LOGDEBUG << "streamIndex: " << streamIndex << " taskIndex: " << taskIndex << std::endl;
        if (!app_.setParameters(streamIndex, taskIndex, params[2])) { result = "Invalid task / stream index"; }
    }
};

/** App method to command the App to shutdown its IO::Stream objects and exit.
 */
struct Shutdown : public AppMethodBase {
    /** Constructor

        \param app the App object to work with

        \param server the server that will execute the method
    */
    Shutdown(App& app, XmlRpc::XmlRpcServer* server) : AppMethodBase(app, server, "shutdown") {}

    /** Perform the XML-RPC request. Implementation of XmlRpcServerMethod interface.

        \param params ignored

        \param result ignored
    */
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result)
    {
        app_.shutdown();
        result = true;
    }
};

struct StateChange : public AppMethodBase {
    /** Constructor

        \param app the App object to work with

        \param server the server that will execute the method
    */
    StateChange(App& app, XmlRpc::XmlRpcServer* server) : AppMethodBase(app, server, "stateChange") {}

    /** Perform the XML-RPC request. Implementation of XmlRpcServerMethod interface.

        \param params arguments to the request. Must contain one integer value,
        the new IO::ProcessingState::Value state to move to.

        \param result ignored
    */
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result)
    {
        IO::ProcessingState::Value state = IO::ProcessingState::Value(int(params[0]));
        app_.postControlMessage(IO::ProcessingStateChangeRequest(state).getWrapped());
        result = true;
    }
};

Logger::Log&
RemoteController::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("runner.RemoteController");
    return log_;
}

bool
RemoteController::start(const QString& serviceName, long threadFlags, long priority)
{
    return RemoteControllerBase::start(serviceName.toStdString(), threadFlags, priority);
}

void
RemoteController::installServices(XmlRpc::XmlRpcServer* server)
{
    static Logger::ProcLog log("installServices", Log());
    LOGINFO << std::endl;
    server->enableIntrospection(true);
    new ClearStats(app_, server);
    new GetChangedParameters(app_, server);
    new GetParameters(app_, server);
    new RecordingChange(app_, server);
    new SetParameters(app_, server);
    new Shutdown(app_, server);
    new StateChange(app_, server);
}

void
RemoteController::serviceNameChanged(const std::string& name)
{
    Logger::ProcLog log("serviceNameChanged", Log());
    LOGERROR << "name conflict: " << name << std::endl;
    app_.setServiceName(name);
}
