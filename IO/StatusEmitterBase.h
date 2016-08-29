#ifndef SIDECAR_IO_STATUSEMITTERBASE_H // -*- C++ -*-
#define SIDECAR_IO_STATUSEMITTERBASE_H

#include <string>

#include "ace/Sched_Params.h"
#include "boost/shared_ptr.hpp"

namespace Logger { class Log; }
namespace XmlRpc { class XmlRpcValue; }

namespace SideCar {
namespace IO {

/** Abstract base class for status emitters. Periodically emits an XML status message to one or more "status
    watcher" objects (see GUI::StatusCollector for an example). Derived classes must define a fillStatus()
    method.

    At first, a StatusEmitter does not emit any status information. Rather, it waits for the appearance of one
    or more status collector services with a particular Zeroconfig service type, at which point the emitter
    starts a timer that will cause it to emit periodic status updates. Whenever there are no active status
    collectors, the emitter will stop its update time and thus stop broadcasting status information.

    Status is contained in an XML-RPC struct structure, which consists of a set of name/value pairs. The
    fillStatus() method adds the name/value pairs to an existing XML-RPC struct value. This result is what the
    Emitter helper class uses when it emits status information to registered listeners.
*/
class StatusEmitterBase
{
public:

    using Ref = boost::shared_ptr<StatusEmitterBase>;

    /** Obtain the log device used by StatusEmitter objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.

	\param name of the Zeroconf service to publish

        \param updateRate the number of seconds between status emissions.
    */
    StatusEmitterBase(const char* emitterType, const char* collectorType, double updateRate);

    /** Destructor. Shutdown any active Emitter object
     */
    virtual ~StatusEmitterBase();

    /** Open a datagram socket and publish connection information.

        \return true if successful
    */
    bool open(long threadFlags = THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED ,
              long priority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Close the datagram socket and shutdown Zeroconf publisher.
     */
    void close();

    /** Change the update rate for the emitter.

        \param updateRate new update rate in seconds
    */
    void setUpdateRate(double updateRate);

    /** Obtain the current update rate of the emitter.

        \return update rate
    */
    double getUpdateRate() const { return updateRate_; }

    void emitStatus();

protected:

    /** Fill in an XML-RPC struct value with name/value pairs that describe the current status of the Algorithm
        we represent. Derived classes should override this method to provide their own status.

        \param status the XML-RPC value to update
    */
    virtual void fillStatus(XmlRpc::XmlRpcValue& status) = 0;

private:
    const char* emitterType_;
    const char* collectorType_;
    std::string name_;
    double updateRate_;

    class Emitter;
    std::unique_ptr<Emitter> emitter_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
