#ifndef SIDECAR_RUNNER_STATUSEMITTER_H // -*- C++ -*-
#define SIDECAR_RUNNER_STATUSEMITTER_H

#include "boost/shared_ptr.hpp"

#include "IO/StatusEmitterBase.h"
#include "IO/ZeroconfRegistry.h"

namespace SideCar {
namespace Runner {

class App;

/** Status emitter for the App class. Periodically emits an XML status message to one or more "status collector"
    objects (see GUI::StatusCollector for an example). Relies on Runner::fillStatus() method for status
    information.
*/
class StatusEmitter : public IO::StatusEmitterBase
{
public:

    using Ref = boost::shared_ptr<StatusEmitter>;

    /** Obtain the Zeroconf type for all StatusEmitter objects.

        \return NULL-terminated C string
    */
    static const char* GetEmitterType()
	{ return IO::ZeroconfRegistry::GetType(IO::ZeroconfRegistry::kRunnerStatusEmitter); }

    /** Obtain the Zeroconf type for all StatusCollector objects associated with this StatusEmitter class.

        \return NULL-terminated C string
    */
    static const char* GetCollectorType()
	{ return IO::ZeroconfRegistry::GetType(IO::ZeroconfRegistry::kRunnerStatusCollector); }

    static Ref Make(App& app, double updateRate = 2.0)
        {Ref ref(new StatusEmitter(app, updateRate)); return ref;}
    
private:

    /** Constructor.

        \param algorithm the algorithm instance we represent

	\param updateRate the number of seconds bbetween status updates
    */
    StatusEmitter(App& app, double updateRate);

    /** Fill in an XML-RPC struct value with name/value pairs that describe the current status of the Algorithm
        we represent. Forwards the request to the Algorithm::fillStatus() method.

        \param status the XML-RPC value to update with status information
    */
    void fillStatus(XmlRpc::XmlRpcValue& status);

private:
    App& app_;
};

} // end namespace Runner
} // end namespace SideCar

/** \file
 */

#endif
