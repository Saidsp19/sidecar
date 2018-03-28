#ifndef SIDECAR_IO_MODULE_H // -*- C++ -*-
#define SIDECAR_IO_MODULE_H

#include <string>

#include "ace/Module.h"

#include "IO/Task.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

class Stream;

/** Derivation of the ACE_Module class for use in Stream objects. Manages a single Task object for message
    processing in the stream. Since our streams only process data in one direction (from stream head to tail),
    we only have one Task to worry about.
*/
class Module : public ACE_Module<ACE_MT_SYNCH, ACE_System_Time_Policy> {
public:
    static Logger::Log& Log();

    /** Constructor for a new Module object.

        \param task the Task object to use for processing of messages flowing down a Stream.

        \param stream the Stream object that will host the new Module object
    */
    Module(const Task::Ref& task, const boost::shared_ptr<Stream>& stream);

    /** Obtain the forward processing Task object assigned to this Module.

        \return Task reference
    */
    const Task::Ref& getTask() const { return task_; }

protected:
    Task::Ref task_;
};

/** Template class for creating Module objects containing Task objects. The template parameter T must be a class
    derived from Task, and it must provide a Make() factory method.
*/
template <typename T>
class TModule : public Module {
public:
    /** Constructor. Create a new ACE module object with one task on the writer side (downstream). The module is
        configured to not delete the writer task.

        \param stream the Stream object hosting the module
    */
    TModule(const boost::shared_ptr<Stream>& stream) : Module(T::Make(), stream) {}

    /** Constructor. Create a new ACE module object with one task on the writer side (downstream). The module is
        configured to not delete the writer task.

        \param stream the Stream object hosting the module

        \param bufferSize the input buffer size to forward to the task Make() factory method.
    */
    TModule(const boost::shared_ptr<Stream>& stream, size_t bufferSize) : Module(T::Make(bufferSize), stream) {}

    /** Obtain a type-cast shared reference to the module's Task object

        \return Task reference
    */
    typename T::Ref getTask() const { return boost::dynamic_pointer_cast<T>(task_); }
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
