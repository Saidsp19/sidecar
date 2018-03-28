#ifndef SIDECAR_IO_STREAM_H // -*- C++ -*-
#define SIDECAR_IO_STREAM_H

#include <string>

#include "ace/Stream.h"
#include "boost/shared_ptr.hpp"

#include "IO/Task.h"

namespace Logger {
class Log;
}
namespace XmlRpc {
class XmlRpcValue;
}

namespace SideCar {
namespace IO {

class StatusBase;
class StatusEmitterBase;

/** A SideCar data stream. Contains one or more Module objects (which contain a Task object) that process data
    given to the head of the stream. Note that the ACE_MT_SYNCH template parameter given to ACE_Stream denotes
    the type of ACE_Task objects the stream holds, and not that there is any locking going on within ACE_Stream.
    Also, abide by the warnings in ACE Stream.h: there is some tricky stuff involved in overriding certain
    methods. This class does no overriding, only extending.
*/
class Stream : public ACE_Stream<ACE_MT_SYNCH> {
public:
    using Ref = boost::shared_ptr<Stream>;
    using StatusEmitterBaseRef = boost::shared_ptr<StatusEmitterBase>;

    static Logger::Log& Log();

    static Ref Make(const std::string& name, const StatusEmitterBaseRef& emitter = StatusEmitterBaseRef())
    {
        Ref ref(new Stream(name, emitter));
        return ref;
    }

    /** Obtain the name of the stream

        \return stream name
    */
    const std::string& getName() const { return name_; }

    /** Obtain the task at a given position from the head of the stream.

        \param index id of the task to get

        \return Task shared reference
    */
    Task::Ref getTask(int index) const;

    /** Fill out a StreamStatus status collection with information from all of the tasks in the stream.

        \param status container to hold the status information
    */
    void fillStatus(StatusBase& status);

    /** Obtain a collection of key/value pairs that describe the parameters changed since startup.

        \param value XML container to hold the changes.
    */
    void getChangedParameters(XmlRpc::XmlRpcValue& value) const;

    /** Force a status emission for the stream.
     */
    void emitStatus() const;

private:
    /** Constructor

        \param name name of the stream
    */
    Stream(const std::string& name, const StatusEmitterBaseRef& emitter) :
        ACE_Stream<ACE_MT_SYNCH>(), name_(name), emitter_(emitter)
    {
    }

    std::string name_;
    StatusEmitterBaseRef emitter_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
