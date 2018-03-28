#ifndef SIDECAR_ALGORITHMS_FANIN_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_FANIN_H

#include "Algorithms/Algorithm.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm FanIn. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class FanIn : public Algorithm {
    using Super = Algorithm;

public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    FanIn(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

private:
    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInputVideo(const Messages::Video::Ref& msg);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInputBinary(const Messages::BinaryVideo::Ref& msg);
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
