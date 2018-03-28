#ifndef SIDECAR_RUNNER_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_RUNNER_DOXYGEN_H

/** \page runner Algorithm Runners

    The application that hosts one or more processing SideCar::IO::Stream instances is known as a \c runner, and
    it is made up of classes found in the SideCar::Runner namespace. A \c runner application requires an XML
    configuration file to describe the contents of the processing streams it will host. It also contains an
    XML-RPC server so that it may accept XMl-RPC requests from an external entity, such as the
    SideCar::GUI::Master application. Finally, it periodically collects data from each of its processing streams
    and emits the result via its SideCar::StatusEmitter object. The SideCar::GUI::Master application contains a
    SideCar::GUI::Master::StatusCollector that accepts status emissions from different \p runner applications
    running on the SideCar LAN, and displays the results to the user.
*/

namespace SideCar {

/** Namespace for entities related to \p runner application.
 */
namespace Runner {
}

} // end namespace SideCar

#endif
