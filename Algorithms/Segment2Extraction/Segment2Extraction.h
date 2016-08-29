#ifndef SIDECAR_ALGORITHMS_SEGMENT2EXTRACTION_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SEGMENT2EXTRACTION_H

#include "Algorithms/Algorithm.h"
#include "Messages/Extraction.h"
#include "Messages/Segments.h"

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms Converts a SegmentList to an Extraction.

   \par Pseudocode:
   - If PowerFlag, create an extraction at the segment's power centroid
   - else create an extraction at the segment's peak.
   - If at least BufferLength messages have been queued, send out a message.

   \par Input Messages:
   - Messages::SegmentMessage of a single target

   \par Output Messages:
   - Messages::Extraction representing the same target

   \par Run-time Parameters:
   bool \b "PowerFlag" = true
   \code
   Specifies how to declare the coordinates of the extraction.
   \endcode

   \par
   positive int \b "BufferLength" = 1
   \code
   Specifies how many extractions to buffer before sending out a message.
   \endcode
*/
class Segment2Extraction : public Algorithm
{
    // Algorithm interface
    //
public:
    Segment2Extraction(Controller& controller, Logger::Log &log);

    bool startup();

private:

    bool process(const Messages::SegmentMessage::Ref& pri);

    // Parameters
    //
    Parameter::BoolValue::Ref powerF;
    Parameter::PositiveIntValue::Ref toBuffer;

    // Other
    //
    Messages::Extractions::Ref extractions;

    float azFactor;
};

}
}

/** \file
 */

#endif
