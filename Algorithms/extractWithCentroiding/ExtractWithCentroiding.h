#ifndef SIDECAR_ALGORITHMS_EXTRACTWITHCENTROIDING_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_EXTRACTWITHCENTROIDING_H

#include "boost/shared_ptr.hpp"

#include "Algorithms/Algorithm.h"
#include "ImageSegmentation.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"
#include "VideoStorage.h"

namespace SideCar {
namespace Algorithms {

class ExtractWithCentroiding : public Algorithm {
public:
    ExtractWithCentroiding(Controller& controller, Logger::Log& log);

    bool startup();

private:
    bool process(const Messages::BinaryVideo::Ref& msg);

    ImageSegmentation m_is;
    VideoStorage m_videoHistory;

    Parameter::DoubleValue::Ref m_centroidRangeMin;
    Parameter::DoubleValue::Ref m_centroidAzMin;
    Parameter::DoubleValue::Ref m_discardRangeMin;
    Parameter::DoubleValue::Ref m_discardAzMin;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
