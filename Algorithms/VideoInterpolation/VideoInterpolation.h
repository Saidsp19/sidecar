#ifndef SIDECAR_ALGORITHMS_VIDEOINTERPOLATION_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_VIDEOINTERPOLATION_H

/* Todo
   - monitor the time/azimuth of the incoming messages for jitter
   (expect delta_t and delta_az/delta_t to be constant)
   use an FIR to estimate the mean values, use another to estimate the variance / standard deviation
*/

#include "Algorithms/Algorithm.h"
#include "Algorithms/PastBuffer.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms Interpolates sparse PRI's at a high angular resolution to produce a dense field of PRI's at
   a lower angular resulution.

   \par Pseudocode:
   - keep the two most recent input messages
   - create any output messages which lie between them

   \par Input Messages:
   - Messages::Video source data

   \par Output Messages:
   - Messages::Video interpolated data

   \par Run-time Parameters:
   positive int \b "MaxInputAzimuth"
   \code
   Specifies the maximum azimuth encoding at the input.
   \endcode

   \par
   positive int \b "MaxOutputAzimuth"
   \code
   Specifies the maximum azimuth encoding at the output.
   \endcode
*/
class VideoInterpolation : public Algorithm
{
public:

    VideoInterpolation(Controller& controller, Logger::Log& log);

    void setInterpolationCount(int value)
	{ interpolationCount_->setValue(value); }

    bool startup();

    bool reset();

private:

    bool process(const Messages::Video::Ref& in);

    // Parameters
    //
    Parameter::PositiveIntValue::Ref interpolationCount_;

    PastBuffer<Messages::Video> past_;
};

} // namespace Algorithm
} // namespace SideCar

/** \file
 */

#endif
