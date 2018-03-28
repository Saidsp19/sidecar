#ifndef SIDECAR_ALGORITHMS_SPREADMOFN_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SPREADMOFN_H

#include "Algorithms/CPIAlgorithm.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"

#include <deque>

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm SpreadMofN. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class SpreadMofN : public CPIAlgorithm {
    using Super = CPIAlgorithm;

public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    SpreadMofN(Controller& controller, Logger::Log& log);

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

private:
    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processCPI();

    // Add attributes here
    //
    Parameter::PositiveIntValue::Ref M_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
