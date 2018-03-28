#ifndef SIDECAR_ALGORITHMS_COMPOSEEXTRACTIONS_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_COMPOSEEXTRACTIONS_H

#include <list>

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm ProcessTracks This task simply receives track messages and passes them
    along.
*/
class ComposeExtractions : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    ComposeExtractions(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

    void processAlarm();

    int timerCount_;

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    // bool processInput(const Messages::Track::Ref& msg);

    bool setupAlarm();

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
