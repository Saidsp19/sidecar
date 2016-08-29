#ifndef SIDECAR_ALGORITHMS_IQFILTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_IQFILTER_H

#include "Algorithms/Algorithm.h"
#include "Messages/Complex.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Conversion filter from I/Q (complex) messages into Video message format. Output messages can be in one of 5
    formats:

    - results of 10 * log10(sqrt(I * I + Q * Q))
    - results of sqrt(I * I + Q * Q)
    - results of I * I + Q * Q
    - actual I samples
    - actual Q samples
    - phase angle atan2(Q, I)

    The IQFilter can accept data in either Video or the new Complex format.
    Which format is used depends on the XML configuration used to start the
    IQFilter. To accept Video messages, use

    \code
    <input type="Video">
    \endcode

    Likewise, to accept Complex messages, add

    \code
    <input type="Complex">
    \endcode

    Note that for either input, there must be a message provider in the XML
    file that has a matching output type, or else the algorithm will fail to
    start up.
*/
class IQFilter : public Algorithm
{
    using Super = Algorithm;
public:

    enum InfoSlot {
	kFilterType = ControllerStatus::kNumSlots,
	kEnabled,
	kNumSlots
    };

    /** Filtering options available via the Master GUI application.
     */
    enum FilterType {
	kMinValue = 0,
	kLogSqrtSumIQSquared = kMinValue,
	kSqrtSumIQSquared,
	kSumIQSquared,
	kISamples,
	kQSamples,
	kPhaseAngle,
	kMaxValue = kPhaseAngle,
	kNumFilterTypes
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    IQFilter(Controller& controller, Logger::Log& log);

    void setFilterType(int filterType)
	{ filterType_->setValue(FilterType(filterType)); }

    /** Implementation of the Algorithm::startup interface. Register run-time parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

private:

    /** Processor for Messages::Video messages. This exists for backwards capability.

        \param msg input message

        \return true if no error
    */
    bool process(const Messages::Video::Ref& msg);

    /** Definition of the enum range for the filterType_ parameter.
     */
    struct FilterTypeEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
	using ValueType = FilterType;
	static ValueType GetMinValue() { return kMinValue; }
	static ValueType GetMaxValue() { return kMaxValue; }
	static const char* const* GetEnumNames();
    };

    using FilterTypeParameter = Parameter::TValue<Parameter::Defs::Enum<FilterTypeEnumTraits>>;

    /** Run-time parameter that determines which filter to use when converting the input message into an output
	message.
    */
    FilterTypeParameter::Ref filterType_;

    Parameter::BoolValue::Ref enabled_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
