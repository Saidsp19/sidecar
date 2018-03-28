#ifndef SIDECAR_ALGORITHMS_RGBCONVERTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_RGBCONVERTER_H

#include "Algorithms/CPIAlgorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include <deque>
#include <vector>
#include <vsip/matrix.hpp>

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm RGBConverter. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class RGBConverter : public CPIAlgorithm {
    using Super = CPIAlgorithm;

public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    RGBConverter(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

    bool cpiSpanChanged(const Parameter::PositiveIntValue& parameter);
    bool processCPI();

    bool resize(int max);

private:
    // Add attributes here
    //
    Parameter::PositiveIntValue::Ref min_;
    Parameter::PositiveIntValue::Ref max_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
