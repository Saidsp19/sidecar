#ifndef SIDECAR_ALGORITHMS_TRACKER_RECEIVE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_TRACKER_RECEIVE_H

#include "Algorithms/Algorithm.h"
#include "Messages/Extraction.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include <cmath>
#include <list>
#include <vector>

// VIADS
#include <Server.h>
#include <TrackMessage.h>
// from radar_post/inc/utils.H
inline double
degreesLat2Meters(const double& latitude_degrees)
{
    return latitude_degrees * 111.3e3;
};

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms
*/
class TrackerReceive : public Algorithm {
public:
    // Algorithm interface
    //
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    TrackerReceive(Controller& controller, Logger::Log& log);

    /** Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    static void* threaded_receiver(void*);

    void init();

private:
};

} // namespace Algorithms
} // namespace SideCar

/** \file
 */

#endif
