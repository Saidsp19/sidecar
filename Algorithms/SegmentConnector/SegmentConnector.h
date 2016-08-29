#ifndef SIDECAR_ALGORITHMS_SEGMENT_CONNECTOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SEGMENT_CONNECTOR_H

#include "Algorithms/Algorithm.h"
#include "Messages/Segments.h"
#include "SegmentTree.h"

namespace SideCar {
namespace Algorithms {

/** Takes a stream of PRISegments and outputs a stream of segments, each representing an extraction blob.
 */
class SegmentConnector : public Algorithm
{
public:
    SegmentConnector(Controller& controller, Logger::Log &log);

    bool startup();
    bool reset();

private:
    bool process(Messages::SegmentMessage::Ref newPRI);

    Messages::SegmentMessage::Ref oldPRI;
    std::list<SegmentTree*>* oldTrees;
    double rangeMin_;
    double rangeFactor_;
};

}} // namespaces

/** \file
 */

#endif
