#ifndef SIDECAR_ALGORITHMS_CPIMARKER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CPIMARKER_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "Time/TimeStamp.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm CPIMarker. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class CPIMarker : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    CPIMarker(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    Messages::Video::Ref last_;

    // Add attributes here
    //

    Parameter::BoolValue::Ref enabled_;
    Parameter::PositiveIntValue::Ref scale_;
    Parameter::DoubleValue::Ref freqMHz_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
