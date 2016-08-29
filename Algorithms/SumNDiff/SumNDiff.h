#ifndef SIDECAR_ALGORITHMS_SUMNDIFF_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SUMNDIFF_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include <deque>

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm SumNDiff. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class SumNDiff : public Algorithm
{
    using Super = Algorithm;
    using VideoMessageBuffer = std::deque<Messages::Video::Ref>;
    using SumVector = std::vector<Messages::Video::DatumType>;
public:

    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
        kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    SumNDiff(Controller& controller, Logger::Log& log);

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

    void radiusChanged(const Parameter::PositiveIntValue& parameter);
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    // Add attributes here
    //
    Parameter::BoolValue::Ref        enabled_;
    Parameter::PositiveIntValue::Ref radius_;

    SumVector sum1_;
    SumVector sum2_;
    VideoMessageBuffer buffer_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
