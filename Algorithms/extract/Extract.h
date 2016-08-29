#ifndef SIDECAR_ALGORITHMS_EXTRACT_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_EXTRACT_H

#include <list>
#include <vector>

#include "boost/shared_ptr.hpp"

#include "Algorithms/Algorithm.h"
#include "Parameter/Parameter.h"
#include "Messages/BinaryVideo.h"

namespace SideCar {
namespace Algorithms {

/** This algorithm takes in thresholded pri's and outputs extraction messages. For now an extraction is declared
    at the center (where the center is simply the average of the min and max az/range) of every contiguous group
    of detections.

*/
class Extract : public Algorithm
{
public:
    class Target;
    using TargetRef = boost::shared_ptr<Target>;
    using TargetList = std::list<TargetRef>;
    using TargetVector = std::vector<TargetRef>;

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Extract(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    bool reset();
    
private:

    /** Input processor for algorithm

        \param mgr object containing the encoded or native data to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::BinaryVideo::Ref& msg);

    /** List of pending target extractions. An pending target is one whose end azimuth has not yet been
	identified.
    */
    TargetList pending_;

    /** List of targets assigned to range gates for the last PRI.
     */
    TargetVector gateTargets_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
