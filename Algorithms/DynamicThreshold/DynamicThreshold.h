#ifndef SIDECAR_ALGORITHMS_DYNAMICTHRESHOLD_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_DYNAMICTHRESHOLD_H

#include "Algorithms/ManyInAlgorithm.h"
#include "Messages/Video.h"
#include "Utils/RunningAverage.h"

namespace SideCar {
namespace Algorithms {

/** Unlike the Threshold algorithm, which is essential a high-pass filter with a constant threshold, this
    algorithm compares samples value in one message with threshold values found in another message, only passing
    those samples that are greater than their corresponding threshold. The resulting output is a BinaryVideo
    message with true values in places where the sample passed the filter.
*/
class DynamicThreshold : public ManyInAlgorithm
{
    using Super = ManyInAlgorithm;
public:

    enum InfoSlots {
        kOperator = Super::kNumSlots,
        kPassPercentage,
        kNumSlots
    };

    /** Op options available via the Master GUI application.
     */
    enum Operator {
        kMinValue,
        kLessThan = kMinValue, 
        kLessThanEqualTo,
        kEqualTo,
        kGreaterThanEqualTo,
        kGreaterThan,
        kMaxValue = kGreaterThan,
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    DynamicThreshold(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    void setOperation(Operator value) { operator_->setValue(value); }

private:

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    ChannelBuffer* makeChannelBuffer(int index, const std::string& name,
                                     size_t maxBufferSize);

    /** Process messages from the input channel buffers

        \returns true if no error; false otherwise
    */
    bool processChannels();

    TChannelBuffer<Messages::Video>* samples_;
    TChannelBuffer<Messages::Video>* thresholds_;
    Utils::RunningAverage passPercentage_;
    
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
