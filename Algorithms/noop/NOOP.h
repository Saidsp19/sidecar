#ifndef SIDECAR_ALGORITHMS_NOOP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_NOOP_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/** An algorithm which does nothing but reads in a Video message type only to immediately write it out.
 */
class NOOP : public Algorithm {
public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    NOOP(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

private:
    /** Input message processor.

        \param msg message to process

        \return true if successful
    */
    bool processVideo(const Messages::Video::Ref& msg);
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
