#ifndef SIDECAR_ALGORITHMS_DOWNCONVERTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_DOWNCONVERTER_H

#include <complex>

#include <vsip/signal.hpp>
#include <vsip/vector.hpp>

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

using ComplexType = std::complex<float>;
using VsipComplexVector = vsip::Vector<ComplexType>;

namespace DownConverterUtils { class Channel; }
/** Documentation for the algorithm %s. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class DownConverter : public Algorithm
{
public:

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    DownConverter(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Example of a message processor that takes in Video data.

        \param msg the message to process
        eturn true if successful, false otherwise
    */
    bool process();

private:

    void maxBufferSizeChanged(const Parameter::PositiveIntValue& parameter);

    // Add attributes here
    //

    DownConverterUtils::Channel* cohoChannel_;
    DownConverterUtils::Channel* rxChannel_;
    
    Parameter::PositiveIntValue::Ref maxBufferSize_;
    Parameter::DoubleValue::Ref alpha_;
    Parameter::BoolValue::Ref enabled_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** ile
 */

#endif
