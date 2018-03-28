#ifndef SIDECAR_ALGORITHMS_SIMPLENOOP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SIMPLENOOP_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm. Please describe what the algorithm does, in layman's terms, and if desired,
    mathematical terms.
*/
class SimpleNOOP : public Algorithm {
public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    SimpleNOOP(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

private:
    /** Example of a message processor that takes in Video data.

        \param msg the message to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::Video::Ref& msg);

    // !!! ADD ATTRIBUTES HERE
    //
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
