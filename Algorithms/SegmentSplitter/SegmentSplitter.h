#ifndef SIDECAR_ALGORITHMS_SEGMENT_SPLITTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SEGMENT_SPLITTER_H

#include "Algorithms/Algorithm.h"
#include "Messages/Segments.h"
#include "Messages/Video.h"
#include "Utils/Buffer/Buffer.h"
#include "Utils/Buffer/BufferRow.h"

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms This algorithm splits apart overlapping extractions.

   \par Pseudocode:
   - Start with an extraction E.
   - Let P be the set of all the local peaks in E.
   - Loop while P is non-empty
   - Take the highest peak p in P; declare it to be a "true" extraction and remove it from P.
   - For all other peaks q in P, if dist(p, q) < k, remove q from P.
   - For each "true" extraction p, output { x in E | dist(p,x)<k }

   \par Input Messages:
   - Messages::Video containing the pre-processed returns
   - Messages::SegmentMessage of (possibly) overlapping targets

   \par Output Messages:
   - Messages::Video passthrough
   - Messages::SegmentMessage of non-overlapping targets

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
   float(0 to 1) \b "overlap" = 0.1
   \code
   Specifies how much overlap to allow between targets.
   \endcode

*/
class SegmentSplitter : public Algorithm
{
    // Algorithm interface
    //
public:
    SegmentSplitter(Controller& controller, Logger::Log& log);
    bool startup();
    bool reset();

private:
    bool processSegment(Messages::SegmentMessage::Ref pri);
    bool processVideo(Messages::Video::Ref msg);

    // Parameters
    //
    Parameter::IntValue::Ref deltaRange;
    Parameter::IntValue::Ref deltaAz;
    Parameter::NormalizedValue::Ref overlap; // maximum allowed overlap
    float minDist; // minimum allowed distance
    void handle_overlap_change(const Parameter::NormalizedValue&);

    // Video buffer
    //
    using VideoT = SideCar::Messages::Video::DatumType;
    Buffer<VideoT> buffer;
    int currentRow;
    Buffer<VideoT>::Row row;
};

}} // namespaces

/** \file
 */

#endif
