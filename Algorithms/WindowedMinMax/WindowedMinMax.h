#ifndef SIDECAR_ALGORITHMS_WINDOWEDMINMAX_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_WINDOWEDMINMAX_H

#include <vector>

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm WindowedMinMax. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class WindowedMinMax : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kProcessor, kNumSlots };

    /** Window processors available via the Master GUI application.
     */
    enum ValueGenerator {
        kMinValue,
        kAverage = kMinValue,
        kMin,
        kMax,
        kMaxMin,
        kMaxMax,
        kMaxValue = kMaxMax,
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    WindowedMinMax(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    int getAverage() const;
    int getMin() const;
    int getMax() const;
    int getMaxMin() const;
    int getMaxMax() const;

    /** Definition of the enum range for the processWindow parameter.
     */
    struct ValueGeneratorEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
        using ValueType = ValueGenerator;
        static ValueType GetMinValue() { return kMinValue; }
        static ValueType GetMaxValue() { return kMaxValue; }
        static const char* const* GetEnumNames();
    };

    using ValueGeneratorParameter = Parameter::TValue<Parameter::Defs::Enum<ValueGeneratorEnumTraits>>;

    void valueGeneratorChanged(const ValueGeneratorParameter& parameter);

    Parameter::BoolValue::Ref enabled_;
    Parameter::PositiveIntValue::Ref windowSize_;
    Parameter::PositiveIntValue::Ref windowCount_;
    Parameter::NonNegativeIntValue::Ref windowOffset_;
    ValueGeneratorParameter::Ref valueGenerator_;

    std::vector<int> windowSums_;
    std::vector<int> windowIndices_;

    using ValueGeneratorProc = int (WindowedMinMax::*)() const;
    ValueGeneratorProc valueGeneratorProc_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
