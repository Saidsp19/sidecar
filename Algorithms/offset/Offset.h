#ifndef SIDECAR_ALGORITHMS_OFFSET_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_OFFSET_H

#include "Algorithms/Algorithm.h"
#include "Parameter/Parameter.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

class Offset : public Algorithm
{
public:
    using DatumType = Messages::Video::DatumType;

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Offset(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    void setOffset(short value) { offset_->setValue(value); }

private:

    /** Implementation of the Algorithm::process interface.

        \param mgr object containing the encoded or native data to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::Video::Ref& msg);

    Parameter::BoolValue::Ref enabled_;
    Parameter::ShortValue::Ref offset_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
