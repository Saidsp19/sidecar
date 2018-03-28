#ifndef SIDECAR_ALGORITHMS_CFAR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CFAR_H

#include "Algorithms/Algorithm.h"
#include "Algorithms/PastBuffer.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

class CFAR : public Algorithm {
public:
    CFAR(Controller& controller, Logger::Log& log);

    bool startup();

    bool reset();

private:
    bool processEstimate(const Messages::Video::Ref& msg);

    bool processVideo(const Messages::Video::Ref& msg);

    Parameter::DoubleValue::Ref alpha_;
    PastBuffer<Messages::Video> videoBuffer_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
