#ifndef SIDECAR_ALGORITHMS_REMOTECONTROLLERBASE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_REMOTECONTROLLERBASE_H

#include <string>

#include "ace/INET_Addr.h"
#include "ace/Task.h"

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"

namespace Logger { class Log; }
namespace XmlRpc { class XmlRpcServer; }

namespace SideCar {
namespace Zeroconf { class Publisher; }
namespace Algorithms {

/** An XML-RPC remote controller abstract base class. Allows an XML-RPC server to run within the ACE framework.
    Derived classes must define an installServices() method to install handlers for external XML-RPC requests.
*/
class RemoteControllerBase : public ACE_Task<ACE_NULL_SYNCH>
{
    using Super = ACE_Task<ACE_NULL_SYNCH>;
public:
    using ZCPublisherRef = boost::shared_ptr<Zeroconf::Publisher>;

    /** Log device for objects of this type.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor

        \param zeroconfType identifier to use to identify the service in mDNS
    */
    RemoteControllerBase(const char* zerconfType);

    /** Destructor. Here only to emit a log message.
     */
    ~RemoteControllerBase();

    /** Start the remote controller. Creates a new XML-RPC server, registers remote methods with the server, and
        then creates a new Zeroconf::Publisher to announce the availability of the XML-RPC server. Spawns a new
        thread that gives time to the XML-RPC server. The thread runs the method svc().

	\param name of the Zeroconf service to publish under

        \return true if successful, false otherwise
    */
    bool start(const std::string& serviceName, long threadFlags = THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED, 
               long priority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Stop the remote controller. Signals the XML-RPC processing thread to stop, then waits for the thread to
        exit. After the thread has terminated, the routine stops and disposes of the Zeroconf::Publisher object,
        and then deletes the XML-RPC server.

        \return true if successful, false otherwise
    */
    bool stop();

    /** Shutdown the controller if it is running. Override of ACE_Task method.

	\param flags indication if task is being closed by a module (non-zero)

	\return 0 if successful, -1 otherwise
    */
    int close(u_long flags = 0) override;

protected:
    
    /** Install the XML-RPC event handlers. Derived classes must define.

	\param server the XML-RPC server to install into
    */
    virtual void installServices(XmlRpc::XmlRpcServer* server) = 0;

    /** Notification from the Zeroconf connection publisher that our name changed due to a conflict. Derived
        classes may override to receive this notification.

	\param name the new service name
    */
    virtual void serviceNameChanged(const std::string& name) {}

private:

    /** Callback invoked when the Zeroconf library finishes the publishing request. Checks to see if the
        published name differs from the desired one and if so invokes serviceNameChanged().

	\param ok true if publishing was successful
    */
    void publisherPublished(bool ok);

    /** Service routine that handles XML-RPC requests. Runs in a separate thread, started from within the
        start() routine.

        \return always returns 0.
    */
    int svc() override;

    const char* zeroconfType_;
    std::string serviceName_;
    ACE_INET_Addr address_;
    boost::scoped_ptr<XmlRpc::XmlRpcServer> rpcServer_;
    ZCPublisherRef connectionPublisher_;
    volatile int running_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
