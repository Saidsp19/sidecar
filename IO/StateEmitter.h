#ifndef SIDECAR_IO_STATEEMITTER_H // -*- C++ -*-
#define SIDECAR_IO_STATEEMITTER_H

#include "boost/scoped_ptr.hpp"
#include <string>

#include "ace/Sched_Params.h"

#include "IO/ZeroconfRegistry.h"

namespace Logger {
class Log;
}
namespace XmlRpc {
class XmlRpcValue;
}

namespace SideCar {
namespace IO {

class StateEmitter : public ZeroconfTypes::StateEmitter {
public:
    /** Obtain the log device used by StatusEmitter objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    StateEmitter();

    /** Destructor. Shutdown any active Emitter object
     */
    virtual ~StateEmitter();

    /** Open a datagram socket and publish connection information.

        \return true if successful
    */
    bool open(const std::string& emitterName, long threadFlags = THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
              long priority = ACE_DEFAULT_THREAD_PRIORITY);

    /** Close the datagram socket and shutdown Zeroconf publisher.
     */
    void close();

    /** Set the value of a key and update the published state of the emitter.
        \param key the key to change
        \param value the value to assign to the key

    */
    void setState(const std::string& key, const std::string& value);

    /** Remove a key from the published state.
        \param key the key to remove
    */
    void removeState(const std::string& key);

    /** Publish the current state to any subscribers.
     */
    void publish();

private:
    class Private;
    boost::scoped_ptr<Private> p_;
};

} // namespace IO
} // end namespace SideCar

/** \file
 */

#endif
