#ifndef SIDECAR_ALGORITHMS_CPIALGORITHM_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CPIALGORITHM_H

#include <deque>

#include "Algorithms/Algorithm.h"
#include "Messages/PRIMessage.h"
#include "Messages/Video.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"
#include "Time/TimeStamp.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm CPIAlgorithm. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class CPIAlgorithm : public Algorithm
{
    using Super = Algorithm;
public:

    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
        kNumSlots
    };

    static QString GetFormattedStats(const IO::StatusBase& status);

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    CPIAlgorithm(Controller& controller, Logger::Log& log,
                 bool enabled, int span);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

    size_t maxMsgSize() const { return maxMsgSize_; }

protected:
    
    using MessageQueue = std::deque<Messages::PRIMessage::Ref>;
    
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);
    
    Parameter::PositiveIntValue::Ref cpiSpan_;
    MessageQueue buffer_;
    
    virtual bool processCPI() = 0;

private:

    virtual bool cpiSpanChanged(const Parameter::PositiveIntValue& parameter) = 0;

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::PRIMessage::Ref& msg);
    bool processInputVideo(const Messages::Video::Ref& msg);
    bool processInputBinary(const Messages::BinaryVideo::Ref& msg);
 
    uint32_t current_prf_code_;
    Time::TimeStamp beginProcessingCPI_;

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::PositiveIntValue::Ref maxCPIBufferSize_;
    Parameter::BoolValue::Ref dropIncompleteCPI_;
    size_t maxMsgSize_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
