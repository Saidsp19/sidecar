#ifndef SIDECAR_ALGORITHMS_SCAN_CORRELATOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SCAN_CORRELATOR_H

#include "Algorithms/Algorithm.h"
#include "Messages/Extraction.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include <cmath>
#include <list>
#include <vector>

// VIADS
#include <Client.h>

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms
*/
class TrackerSend : public Algorithm {
public:
    // Algorithm interface
    //
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    TrackerSend(Controller& controller, Logger::Log& log);

    /**

       \param mgr object containing the encoded or native data to process

       \return true if successful, false otherwise
    */
    bool process(const Messages::Extractions::Ref& msg, uint port);

    bool startup();

private:
    Client* client;
    bool ppiInfoSent;
};

} // namespace Algorithms
} // namespace SideCar

/** \file
 */

#endif
