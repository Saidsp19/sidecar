#ifndef SIDECAR_ALGORITHMS_TSPI_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_TSPI_H

#include "Algorithms/Algorithm.h"
#include "Messages/TSPI.h"

namespace SideCar {
namespace Algorithms {

/** Simple algorithm that reads/writes TSPI messages, possibly recording them.
 */
class TSPI : public Algorithm
{
public:

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    TSPI(Controller& controller, Logger::Log& log);

    bool startup();

private:
    
    /** Implementation of the Algorithm::process interface.

        \param mgr object containing the encoded or native data to process

        \param channel identifier of the device that provided the data

        \return true if successful, false otherwise
    */
    bool process(const Messages::TSPI::Ref& in);
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
