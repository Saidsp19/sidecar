#ifndef SIDECAR_RUNNER_REMOTECONTROLLER_H // -*- C++ -*-
#define SIDECAR_RUNNER_REMOTECONTROLLER_H

#include "QtCore/QString"

#include "Algorithms/RemoteControllerBase.h"
#include "IO/ZeroconfRegistry.h"
#include "XMLRPC/XmlRpcServerMethod.h"

namespace SideCar {
namespace Runner {

class App;

/** A remote controller class for App objects.
 */
class RemoteController : public Algorithms::RemoteControllerBase, public IO::ZeroconfTypes::RunnerRemoteController {
public:
    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor

        \param app reference to the App object to control
    */
    RemoteController(App& app) : RemoteControllerBase(GetZeroconfType()), app_(app) {}

    /** Start the XML-RPC service.

        \param serviceName the name to advertise

        \param threadFlags flags controlling the service thread

        \param priority priority of the service thread

        \return true if started, false otherwise
    */
    bool start(const QString& serviceName, long threadFlags = THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
               long priority = ACE_DEFAULT_THREAD_PRIORITY);

private:
    /** Notification that the Zeroconf service name changed due to a name conflict.

        \param name the new Zeroconf service name
    */
    void serviceNameChanged(const std::string& name);

    /** Obtain the Zeroconf type for the remote controller objects. Implementation of RemoteControllerBase API.

        \return Zerconf type
    */
    const char* getZeroconfType() const { return GetZeroconfType(); }

    /** Install the XML/RPC event handlers. Implementation of the RemoteControllerBase API.
     */
    void installServices(XmlRpc::XmlRpcServer* server);

    App& app_;
};

} // end namespace Runner
} // end namespace SideCar

/** \file
 */

#endif
