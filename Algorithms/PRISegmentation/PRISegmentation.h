#ifndef SIDECAR_ALGORITHMS_PRI_SEGMENTATION_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_PRI_SEGMENTATION_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms Run-length encodes thresholded video.

   \par Pseudocode:
   - azimuth=video.azimuth
   - for each run of cells achieving the threshold, create a new Messages::Segment at (azimuth, start range, stop range)

   \par Input Messages:
   - Messages::Video to be thresholded

   \par Output Messages:
   - Messages::SegmentMessage run-length encoding of the thresholded video

   \par Run-time Parameters:
   int \b "threshold" = 5000
   \code
   Specifies what threshold must be met to declare a detection.
   \endcode
*/
class PRISegmentation : public Algorithm {
public:
    PRISegmentation(Controller& controller, Logger::Log& log);

    bool startup();
    bool reset();

private:
    bool process(const Messages::Video::Ref& pri);

    Parameter::IntValue::Ref threshold_;
    bool busy_;
};

} // namespace Algorithms
} // namespace SideCar

/** \file
 */

#endif
