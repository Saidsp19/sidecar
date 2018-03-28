#ifndef SIDECAR_ALGORITHMS_DELAY_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_DELAY_H

#include "boost/scoped_ptr.hpp"

#include "Algorithms/Algorithm.h"
#include "Messages/BinaryVideo.h"
#include "Messages/PRIMessage.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include <deque>

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm Delay. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class Delay : public Algorithm {
    using Super = Algorithm;
    using MessageQueue = std::deque<Messages::PRIMessage::Ref>;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Delay(Controller& controller, Logger::Log& log);

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

    /** Process messages from specified channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInputVideo(const Messages::Video::Ref& msg);
    bool processInputBinary(const Messages::BinaryVideo::Ref& msg);

    MessageQueue buffer_;
    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::PositiveIntValue::Ref delay_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
