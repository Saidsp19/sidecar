#ifndef SIDECAR_ALGORITHMS_RAWPRI_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_RAWPRI_H

#include "Algorithms/Algorithm.h"
#include "Messages/RawVideo.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Simple algorithm that transforms simply converts data from RIU format into normal PRI format.
 */
class RawPRI : public Algorithm
{
    using Super = Algorithm;
public:

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    RawPRI(Controller& controller, Logger::Log& log);

    bool startup();

private:
    
    /** Implementation of the Algorithm::process interface.

        \param mgr object containing the encoded or native data to process

        \param channel identifier of the device that provided the data

        \return true if successful, false otherwise
    */
    bool process(const Messages::RawVideo::Ref& in);
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
