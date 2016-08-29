#ifndef SIDECAR_ALGORITHMS_PRI_SEGMENTATION_H
#define SIDECAR_ALGORITHMS_PRI_SEGMENTATION_H

#include "Algorithms/Algorithm.h"
#include "Messages/Segments.h"
#include "Messages/Video.h"
#include "Utils/Buffer/Buffer.h"
#include "Utils/Buffer/BufferRow.h"


namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms Calculates some basic statistics an extraction.

   \par Pseudocode:
   - Find the extraction's peak
   - Find the extraction's centroid
   - Find the extraction's total power and power centroid
   - From the power centroid, find the drop-off points.  A drop-off is declared if any of the following are met.
   - p[n] < MinPower*mean(p)
   - p[n] < MaxDrop*p[n-1]
   - exceeding the target's extent

   \par Input Messages:
   - Messages::Video containing the video data
   - Messages::SegmentMessage of individual targets

   \par Output Messages:
   - Messages::Video passthrough
   - Messages::SegmentMessage of individual targets with valid statistics

   \par Run-time Parameters:
   positive int \b "azimuth/2" = 6
   \code
   Specifies half of the nominal target's azimuth extent.
   \endcode

   \par
   positive int \b "range/2" = 3
   \code
   Specifies half of the nominal target's range extent.
   \endcode

   \par
   float(0 to 1) \b "MaxRangeDrop" = 0.5
   \code
   Specifies the maximum rate of decline along the range axis.
   \endcode

   \par
   float(0 to 1) \b "MaxAzimuthDrop" = 0.5
   \code
   Specifies the maximum rate of decline along the azimuth axis.
   \endcode

   \par
   float(0 to 1) \b "MinPower" = 0.5
   \code
   Specifies what fraction of the extraction's average power defines its edges.
   \endcode
*/
class SegmentStats : public Algorithm
{
    // Algorithm interface
    //
public:
    SegmentStats(Controller& controller, Logger::Log& log);

    bool startup();

private:
    /// Stream of segments to process
    bool processSegmentMessage(Messages::SegmentMessage::Ref);

    /// Buffer the video information
    bool processVideo(Messages::Video::Ref);

    using VideoT = SideCar::Messages::Video::DatumType;

    // Processing paths
    //
    int findRangeEnd(size_t, size_t, int, int, VideoT);
    int findAzEnd(size_t, size_t, int, int, size_t, VideoT);

    // Parameters
    //
    Parameter::IntValue::Ref deltaRange;
    Parameter::IntValue::Ref deltaAz;
    Parameter::NormalizedValue::Ref maxRangeDrop;
    Parameter::NormalizedValue::Ref maxAzDrop;
    Parameter::NormalizedValue::Ref minPower;

    // Radar settings
    //
    size_t azimuthEncodings;
    size_t rangeGates;

    // Video buffer
    //
    Buffer<VideoT> buffer;
    int currentRow;
    Buffer<VideoT>::Row row;
};

}} // namespaces

/** \file
 */

#endif
