#ifndef SIDECAR_IO_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_IO_DOXYGEN_H

/** \page io SideCar Component Overview

    The SideCar system is built on top of the <a href="http://www.cs.wustl.edu/~schmidt/ACE.html">Adaptive
    Communication Environment</a> (ACE) object-oriented network programming toolkit. In particular, it relies on
    ACE classes to perform nearly all of the low-level socket and file IO in a system independent way -- ACE
    runs on a wide variety of Unix platforms, Apple MacOS X, and MS Windows systems.

    The SideCar framework contains light-weight derivation of the following powerful ACE classes:

    - ACE_Stream (SideCar::IO::Stream)
    - ACE_Module (SideCar::IO::Module and SideCar::IO::TModule)
    - ACE_Task (SideCar::IO::Task)

    An ACE_Stream object contains one or more ACE_Module objects, which in turn contain one or two ACE_Task
    objects, the number depending on whether the stream supports unidirectional or bidirectional message flow.

    Messages flow from task to task via their \c put() and \c put_next() methods. If a task processes data in a
    separate thread, then the \c put() method usually places the incoming message into an ACE_Message_Queue that
    contains appropriate mutex protection for multi-threaded access. The \c put_next() method forwards a message
    to the next entity linked to the task.

    The SideCar infrastructure uses the ACE stream mechanics without modification. However, its own
    SideCar::IO::Task class adds a runtime state graph that properly manages transitions from on run state to
    another (see SideCar::IO::ProcessingState). SideCar::IO::Task also manages and manipulates the message flow
    by treating control and data messages separately, giving them each their own control flow within
    SideCar::IO::Task. This allows SideCar::IO::Task and its decendents to deliver and process control messages
    and data messages in different ways. Here is the call chains for control messages:

    - SideCar::IO::Task::put() entry point into message processing
    - SideCar::IO::Task::put_next() forward the control message to the next task in the stream
    - SideCar::IO::Task::deliverControlMessage() accept the message
    - SideCar::IO::Task::processControlMessage() invoke a 'do' method based on the type of control message
    given. See SideCar::IO::ControlMessage for an enumeration of control types. Each message type has its own
    handler in SideCar::IO::Task (eg. doProcessingStateChange(), doRecordingStateChange()) which derived
    classes may override.

    Handling of data messages is a little more complex. Firstly, SideCar::IO::Task does not do data processing;
    derived classes must define the deliverDataMessage() and processDataMessage() methods. Also, data messages
    have a list of recipients, placed on the messsage when the message was first emitted from a task. The list
    of recipients identifies which tasks in a processing stream should see the messsage. The big benefit of this
    is that if a task is not interested in a message, the message will never reach its message queue for
    processing, thus reducing the load of any service thread running to process data from the message queue and
    removing the need to mutex lock the message queue for message delivery.

    However, if a task is the next recipient of a message, the call flow is as follows:

    - SideCar::IO::Task::put() entry point into message processing (same as for control flow)
    - SideCar::IO::Task::deliverDataMessage() perform message delivery. For most classed derived from
    SideCar::IO::Task (eg. SideCar::Algorithms::Controller), messages are simply added to its message queue,
    and a separate thread will perform the actual work on the message when it gets time to do so.

    The SideCar::IO::Stream, SideCar::IO::Module, and SideCar::IO::Task classes all support status reporting,
    the results of which are periodically sent out to interested parties such as the SideCar::GUI::Master
    application. A stream collects status from each of its modules, which in turn collect status from its task
    object (SideCar streams are unidirectional in data flow; thus a SideCar::IO::Module only contains one
    SideCar::IO::Task object.)
*/

namespace SideCar {

/** Namespace for entities that handle SideCar data I/O. The SideCar system provides a publisher/subscriber
    infrastructure that supports auto-discovery of services using Apple's Zeroconf system (see the documention
    of the SideCar::Zeroconf namespace for additional details).

    The ACE classes handle nearly all of the low-level I/O details, providing a nice abstraction layer for the
    SideCar classes. The SideCar system provides file and socket I/O source and sink end points. Interfacing
    with the ACE infrastructure occurs in the IO/Readers.h and IO/Writers.h files.
*/
namespace IO {
}

} // end namespace SideCar

#endif
