#ifndef SIDECAR_ALGORITHMS_NCINTEGRATE_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_NCINTEGRATE_H

#include <vector>

#include "Algorithms/Algorithm.h"
#include "Algorithms/PastBuffer.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** This algorithm averages a block of pulses around the input pulse. It takes the run-time parameter numPulses.
    It waits until it receives 'numPulses' pri's, after which it returns the average of the last 'numPulses'
    pri's at the output.

*/
class NCIntegrate : public Algorithm
{
public:
    using DatumType = Messages::Video::DatumType;
    using RunningAverageVector = std::vector<int32_t>;

    enum InfoSlot {
	kNumPRIs = ControllerStatus::kNumSlots,
	kIQValues,
	kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    NCIntegrate(Controller& controller, Logger::Log& log);

    /** Get the current number of pulses to integrate.

        \return current setting
    */
    int getNumPulses() const { return numPulses_->getValue(); }

    /** Set the number of pulses to integrate.

        \param numPulses new number of pulses
    */
    void setNumPulses(int numPulses) { numPulses_->setValue(numPulses); }

    void setIQValues(bool value) { iqValues_->setValue(value); }

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    bool reset();

private:

    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Override of Algorithm::setInfoSlots(). Stores XML representation of cancellation statistics ino the
        given XML-RPC container.

        \param status XML-RPC container to hold the stats
    */
    void setInfoSlots(IO::StatusBase& status);

    /** Process a PRI message.

        \param mgr Video message to process

        \return true if successful, false otherwise
    */
    bool process(Messages::Video::Ref msg);

    /** Notification that the averaging window size has been changed by an external entity.

        \param value new value
    */
    void numPulsesChanged(const Parameter::PositiveIntValue& value);

    /** Notification that the IQ values setting has changed.

        \param value new value
    */
    void iqValuesChanged(const Parameter::BoolValue& value);

    Parameter::BoolValue::Ref enabled_;
    Parameter::BoolValue::Ref iqValues_;
    Parameter::PositiveIntValue::Ref numPulses_;
    PastBuffer<Messages::Video> in_;
    RunningAverageVector runningAverage_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
