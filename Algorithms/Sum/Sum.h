#ifndef SIDECAR_ALGORITHMS_SUM_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SUM_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include <deque>

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm Sum. This algorithm returns the sum of the range bin values for the last N
    messages.
*/
class Sum : public Algorithm
{
    using Super = Algorithm;
    using VideoMessageBuffer = std::deque<Messages::Video::Ref>;
public:

    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
        kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Sum(Controller& controller, Logger::Log& log);

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
    void bufferSizeChanged(const Parameter::PositiveIntValue& parameter);

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::PositiveIntValue::Ref bufferSize_;

    std::vector<Messages::Video::DatumType> sums_;
    VideoMessageBuffer buffer_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
