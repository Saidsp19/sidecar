#ifndef SIDECAR_ALGORITHMS_SCALE_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SCALE_H

#include "Algorithms/Algorithm.h"
#include "Parameter/Parameter.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

class Scale : public Algorithm
{
public:
    using DatumType = Messages::Video::DatumType;

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Scale(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    void setScale(double value) { scale_->setValue(value); }
    
private:

    /** Process next PRI message.

        \param msg message to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::Video::Ref& msg);

    Parameter::BoolValue::Ref enabled_;
    Parameter::DoubleValue::Ref scale_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
