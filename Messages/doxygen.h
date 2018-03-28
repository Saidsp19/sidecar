#ifndef SIDECAR_MESSAGES_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_MESSAGES_DOXYGEN_H

/** \page messages Messages Directory

    Documentation for the Messages directory.
*/

namespace SideCar {

/** SideCar data message classes. The base class for all data messages is the Messages::Header class. It defines
    the common attributes shared by all other data message classes. The primary sample data class is
    Messages::Video which contains short integer samples. This class also hosts interleaved complex data (I+Q),
    but with a length twice the size of a non-complex data message.
*/
namespace Messages {
}

} // end namespace SideCar

#endif
