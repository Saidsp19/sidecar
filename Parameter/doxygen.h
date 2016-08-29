#ifndef SIDECAR_PARAMETER_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_PARAMETER_DOXYGEN_H

/** \page parameter Parameter Directory

    Documentation for the Parameter directory. See the Parameter namespace documentation for additional
    information.
*/

namespace SideCar {

/** Definitions for algorithm runtime parameter settings. A runtime parameter is initialized with a value when
    an algorithm starts up, and it may be changed while an algorithm is running via XML/RPC.

    Parameter objects support reading and writting to/from files and sockets, in both binary and XML format.
*/
namespace Parameter {}

} /// end namespace SideCar

#endif
