#ifndef SIDECAR_ALGORITHMS_SLIDINGOP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SLIDINGOP_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** This algorithm performs an operation on a set of values encapsulated in a sliding window. The window is
    defined for each bin by an offset and a window length. Consequently, you can define the previous n bins for
    bin i with offset = -(n+1) and windowSize = n, a window of size n where bin i is the middle with offset =
    -n/2 and windowSize = n, or the next n bins of i with offset = 1 and windowSize = n. The available
    operations are listed below.
   
*/
class SlidingOp : public Algorithm
{
    using Super = Algorithm;
public:

    enum InfoSlot {
        kEnabled = ControllerStatus::kNumSlots,
        kInitialOffset,
        kWindowSize,
        kEmptyValue,
        kOperation,
        kNumSlots
    };

    /** Op options available via the Master GUI application.
     */
    enum Operation {
        kMinValue,
        kSumOp = kMinValue, 
        kProdOp,
        kAverageOp,
        kMedianOp,
        kMinOp,
        kMaxOp,
        kMaxValue = kMaxOp
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    SlidingOp(Controller& controller, Logger::Log& log);

    bool startup();

    void setInitialOffset(int value) { initialOffset_->setValue(value); }

    void setWindowSize(size_t value) { windowSize_->setValue(value); }

    void setEmptyValue(int value) { emptyValue_->setValue(value); }

    void setOperation(Operation value) { operation_->setValue(value); }

private:

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    /** Definition of the enum range for the operation_ parameter.
     */
    struct OperationEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
        using ValueType = Operation;
        static ValueType GetMinValue() { return kMinValue; }
        static ValueType GetMaxValue() { return kMaxValue; }
        static const char* const* GetEnumNames();
    };

    using OperationParameter = Parameter::TValue<Parameter::Defs::Enum<OperationEnumTraits>>;

    // Runtime attributes
    //
    Parameter::BoolValue::Ref enabled_; ///< If false, just pass on msgs
    Parameter::IntValue::Ref initialOffset_;	  ///< Offset for first window
    Parameter::PositiveIntValue::Ref windowSize_; ///< # samples in window
    Parameter::IntValue::Ref emptyValue_;	  ///< Non-existent value

    /** Run-time parameter that determines the operation to perform within each window.
     */
    OperationParameter::Ref operation_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
