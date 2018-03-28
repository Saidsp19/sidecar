#ifndef SIDECAR_ALGORITHMS_TRIMMER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_TRIMMER_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm Trimmer. Removes a configurable number of samples from the start of a
    message, and make sure that the message is not any larger than a maximum value.
*/
class Trimmer : public Algorithm {
public:
    enum InfoSlot { kEnabled = ControllerStatus::kNumSlots, kFirstSample, kMaxSampleCount, kComplexSamples, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Trimmer(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    void setFirstSample(size_t value);

    void setMaxSampleCount(size_t value);

    void setComplexSamples(bool value);

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Example of a message processor that takes in Video data.

        \param msg the message to process

        \return true if successful, false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    void endParameterChanges();

    size_t firstIndex_;
    size_t lastIndex_;
    bool isComplex_;

    Parameter::NonNegativeIntValue::Ref firstSample_;
    Parameter::NonNegativeIntValue::Ref maxSampleCount_;
    Parameter::BoolValue::Ref complexSamples_;
    Parameter::BoolValue::Ref enabled_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
