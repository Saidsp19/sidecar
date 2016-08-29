#ifndef SIDECAR_ALGORITHMS_CPISIMPLEOP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CPISIMPLEOP_H

#include "Algorithms/CPIAlgorithm.h"
#include "Messages/PRIMessage.h"
#include "Messages/Video.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm CPISimpleOp. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class CPISimpleOp : public CPIAlgorithm
{
    using Super = CPIAlgorithm;
public:

    enum InfoSlots {
        kOperator = Super::kNumSlots,
        kNumSlots
    };

    enum ChannelDataType {
	kVideo,
	kBinaryVideo,
	kUnknownType,
	kNumChannelTypes = kUnknownType
    };

    /** Operator options available via the Master GUI application.
     */
    enum Operator {
        kMinValue,
        kSumOp = kMinValue, 
        kProdOp,
        kMinOp,
        kMaxOp,
        kLastVideoOp = kMaxOp,
        kAndOp,
        kOrOp,
        kMaxValue = kOrOp
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    CPISimpleOp(Controller& controller, Logger::Log& log);

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

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processCPI();
    bool processVideoCPI();
    bool processBinaryVideoCPI();

    /** Definition of the enum range for the domain_ parameter.
     */
    struct OperatorEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
        using ValueType = Operator;
        static ValueType GetMinValue() { return kMinValue; }
        static ValueType GetMaxValue() { return kMaxValue; }
        static const char* const* GetEnumNames();
    };

    using OperatorParameter = Parameter::TValue<Parameter::Defs::Enum<OperatorEnumTraits>>;

    bool cpiSpanChanged(const Parameter::PositiveIntValue& parameter);
    bool operatorChanged(const OperatorParameter& parameter);

    ChannelDataType dataType_;
    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    OperatorParameter::Ref       operator_;

};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
