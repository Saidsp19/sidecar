#ifndef SIDECAR_ALGORITHMS_DESPECKLE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_DESPECKLE_H

#include "Algorithms/Algorithm.h"
#include "Algorithms/PastBuffer.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/** Attempts to suppress noise using a modified median filter. Delays the output by 2 PRIs.

    \par Pseudocode:
    In the 3x3 block about each cell,
    - Find the median of the samples not in this PRI
    - Find the variance of those samples about their median
    - If the center > median + k*variance, set center=median

    \par Input Messages:
    - Messages::Video containing the video data

    \par Output Messages:
    - Messages::Video containing the despeckled data

    \par Run-time Parameters:
    float \b "k" = 2
    \code
    Specifies the allowed variance.
    \endcode
*/
class Despeckle : public Algorithm
{
public:

    Despeckle(Controller& controller, Logger::Log &log);

    bool startup();

    bool reset();

    void setVarianceMultiplier(double value) { varianceMultiplier_->setValue(value); }

private:

    bool process(Messages::Video::Ref);

    using VideoT = Messages::Video::DatumType;

    // Parameters
    //
    Parameter::DoubleValue::Ref varianceMultiplier_;

    // Past buffer
    //
    PastBuffer<Messages::Video> past_;

    static void Sort(VideoT& a, VideoT& b) { if (a > b) std::swap(a, b); }
};

}} // namespaces

/** \file
 */

#endif
