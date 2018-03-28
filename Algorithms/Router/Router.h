#ifndef SIDECAR_ALGORITHMS_ROUTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_ROUTER_H

#include "Algorithms/Algorithm.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm Router. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class Router : public Algorithm {
    using Super = Algorithm;

public:
    enum Channel {
        kMinValue = 1,
    };

    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kInputChannel, kOutputChannel, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Router(Controller& controller, Logger::Log& log);

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

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Header::Ref& msg);

    void endParameterChanges();

    struct ChannelEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
        using ValueType = Channel;
    };

    using ChannelParameter = Parameter::TValue<Parameter::Defs::DynamicEnum<ChannelEnumTraits>>;

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    ChannelParameter::Ref inputChannel_;
    size_t inputChannelIndex_;
    ChannelParameter::Ref outputChannel_;
    size_t outputChannelIndex_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
