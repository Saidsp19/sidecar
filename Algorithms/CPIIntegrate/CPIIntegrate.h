#ifndef SIDECAR_ALGORITHMS_CPIINTEGRATE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CPIINTEGRATE_H

#include <deque>
#include <vector>

#include "Algorithms/CPIAlgorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm CPIIntegrate. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class CPIIntegrate : public CPIAlgorithm {
    using Super = CPIAlgorithm;

public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    CPIIntegrate(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

    /** Implementation of CPIAlgorithm::processCPI interface. Process a batch of PRIs and treat them as a CPI.
     */
    bool processCPI();

    bool cpiSpanChanged(const Parameter::PositiveIntValue& parameter);
    void numCPIsChanged(const Parameter::PositiveIntValue& parameter);

    bool reset();

private:
    // Add attributes here
    //
    /** Number of CPIs to integrate over
     */
    Parameter::PositiveIntValue::Ref numCPIs_;

    /** Buffer to hold CPIs as they arrive in piecemeal as PRIs
     */
    std::deque<MessageQueue*> cpis_;
    /** Running 2D buffer of sums for computing the averages
     */
    std::vector<std::vector<float>> vals_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
