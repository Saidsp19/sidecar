#ifndef SIDECAR_ALGORITHMS_CLAMP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CLAMP_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** This algorithm "clamps" sample values to a given range [min, max]. If a sample value falls below the min, it
    is set to the min. If a sample value falls above the max, it is set to the max.
*/
class Clamp : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kMin, kMax, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Clamp(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Set the min and max clamping values

        \param min lower clamping value

        \param max upper clamping value
    */
    void setRange(int min, int max);

    /** Set the lower clamping value

        \param value new value to use
    */
    void setMinValue(int value);

    /** Set the upper clamping value

        \param value new value to use
    */
    void setMaxValue(int value);

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::IntValue::Ref min_;
    Parameter::IntValue::Ref max_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
