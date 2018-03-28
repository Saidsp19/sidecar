#ifndef SIDECAR_ALGORITHMS_SHIFTBINS_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SHIFTBINS_H

#include "Algorithms/Algorithm.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm ShiftBins. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class ShiftBins : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kShift, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    ShiftBins(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    void setShift(int value) { shift_->setValue(value); }

private:
    template <typename T>
    bool process(const typename T::Ref& msg)
    {
        static Logger::ProcLog log("process", getLog());
        if (!enabled_) {
            LOGDEBUG << "not enabled" << std::endl;
            return send(msg);
        }

        LOGINFO << std::endl;

        size_t size = msg->size();
        int shift = shift_->getValue();
        if (::abs(shift) >= size) {
            getController().setError("Shift value too large");
            LOGERROR << "shift value too large" << std::endl;
            return true;
        }

        // Create a new message to hold the output of what we do. Note that although we pass in the input
        // message, the new message does not contain any data.
        //
        typename T::Ref out(T::Make("ShiftBins", msg));

        // By default the new message has no elements. Either append them or resize.
        //
        out->resize(size, 0);

        if (shift < 0) {
            // Left shift. The |shift| values at the end will be 0.
            //
            std::copy(msg->begin() - shift, msg->end(), out->begin());
        } else {
            // Right shift. The |shift| values at the beginning will be 0.
            //
            std::copy(msg->begin(), msg->end() - shift, out->begin() + shift);
        }

        bool rc = send(out);
        LOGDEBUG << "rc: " << rc << std::endl;
        return rc;
    }

    bool processInputVideo(const Messages::Video::Ref& msg);

    bool processInputBinary(const Messages::BinaryVideo::Ref& msg);

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::IntValue::Ref shift_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
