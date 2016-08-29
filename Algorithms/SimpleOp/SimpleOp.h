#ifndef SIDECAR_ALGORITHMS_SIMPLEOP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SIMPLEOP_H

#include "Algorithms/ChannelBuffer.h"
#include "Algorithms/ManyInAlgorithm.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm SimpleOp. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/
class SimpleOp : public ManyInAlgorithm
{
    using Super = ManyInAlgorithm;
    using VideoChannelBuffer = TChannelBuffer<Messages::Video>;

public:

    enum InfoSlot {
	kOperator = Super::kNumSlots,
	kNumSlots
    };
    
    /** Op options available via the Master GUI application.
     */
    enum Operator {
	kMinValue,
	kSumOp = kMinValue, 
	kDiffOp,
	kProdOp,
	kMinOp,
	kMaxOp,
	kMaxValue = kMaxOp
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    SimpleOp(Controller& controller, Logger::Log& log);

    void setOperation(Operator op) { operator_->setValue(op); }

private:

    /** Create a new ChannelBuffer object for Video messages. Also creates and registers a BoolParameter object
        for runtime editing of the enabled state of the ChannelBuffer object.

        \param channelIndex the index of the channel to create

        \param maxBufferSize the maximum number of messages to buffer

        \return new ChannelBuffer object
    */
    ChannelBuffer* makeChannelBuffer(int channelIndex, const std::string& name, size_t maxBufferSize);

    /** Implementation of ManyInAlgorithm::processChannels() method. Creates a mesage with summed sample values
        and emits it.
        
        \return 
    */
    bool processChannels();

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.
        
        \return true if successful, false otherwise
    */
    bool startup();

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Definition of the enum range for the domain_ parameter.
     */
    struct OperatorEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
	using ValueType = Operator;
	static ValueType GetMinValue() { return kMinValue; }
	static ValueType GetMaxValue() { return kMaxValue; }
	static const char* const* GetEnumNames();
    };

    using OperatorParameter = Parameter::TValue<Parameter::Defs::Enum<OperatorEnumTraits>>;

    /** Run-time parameter that determines the domain to perform the matched filter.
     */
    OperatorParameter::Ref operator_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
