#ifndef SIDECAR_ALGORITHMS_NCINTEGRATE_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_NCINTEGRATE_H

#include "Algorithms/Algorithm.h"
#include "Algorithms/PastBuffer.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/** This algorithm averages a block of pulses around the input pulse. It takes the run-time parameter numPulses.
    It waits until it receives 'numPulses' pri's, after which it returns the average of the last 'numPulses'
    pri's at the output.

*/
class MTI2 : public Algorithm
{
public:
    using DatumType = Messages::Video::DatumType;

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    MTI2(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    bool reset();

private:

    /** Implementation of the Algorithm::process interface.

        \param mgr object containing the encoded or native data to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::Video::Ref& msg);

    PastBuffer<Messages::Video> past_;
    Parameter::BoolValue::Ref enabled_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
