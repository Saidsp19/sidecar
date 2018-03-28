#ifndef SIDECAR_ALGORITHMS_EDGEDETECTOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_EDGEDETECTOR_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm EdgeDetector. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class EdgeDetector : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEdge = ControllerStatus::kNumSlots, kDetectedCount, kExaminedCount, kNumSlots };

    /** Edge
     */
    enum Edge {
        kMinValue,
        kLeading = kMinValue,
        kTrailing,
        kMaxValue = kTrailing,
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    EdgeDetector(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    bool reset();

    void setDetectionType(Edge type);

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    /** Definition of the enum range for the processWindow parameter.
     */
    struct EdgeEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
        using ValueType = Edge;
        static ValueType GetMinValue() { return kMinValue; }
        static ValueType GetMaxValue() { return kMaxValue; }
        static const char* const* GetEnumNames();
    };

    using EdgeParameter = Parameter::TValue<Parameter::Defs::Enum<EdgeEnumTraits>>;

    EdgeParameter::Ref edge_;

    size_t detectedCount_;
    size_t examinedCount_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
