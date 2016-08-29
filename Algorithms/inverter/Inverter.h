#ifndef SIDECAR_ALGORITHMS_INVERTER_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_INVERTER_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Simple algorithm that inverts data found in a PRIMessage object. It has two runtime configurable settings: -
    min -- minimum expected value in a PRIMessage - max -- maximum expected value in a PRIMessage The conversion
    is simply (max - value) + min for all values in a message. Note that there is no clamping of values to min
    or max, so if the value is outside of that range it will also be outside of the inverted domain.
*/
class Inverter : public Algorithm
{
public:
    using DatumType = Messages::Video::DatumType;

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Inverter(Controller& controller, Logger::Log& log);

    /** Get the current minimum value setting

        \return current setting
    */
    DatumType getMin() const { return min_->getValue(); }

    /** Get the current maximum value setting

        \return current setting
    */
    DatumType getMax() const { return max_->getValue(); }

    /** Set the minimum value expected on input.

        \param min new minimum value
    */
    void setMin(DatumType min) { min_->setValue(min); }

    /** Set the maximum value expected on input.

        \param max new maximum value
    */
    void setMax(DatumType max) { max_->setValue(max); }

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

private:

    /** Implementation of the Algorithm::process interface.

        \param mgr object containing the encoded or native data to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::Video::Ref& msg);

    /** Routine given to std::transform that inverts a given value.

        \param value value to invert

        \return inversion result
    */
    DatumType invert(DatumType value) const;

    Parameter::IntValue::Ref min_; ///< Runtime parameter for the min value
    Parameter::IntValue::Ref max_; ///< Runtime parameter for the max value
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
