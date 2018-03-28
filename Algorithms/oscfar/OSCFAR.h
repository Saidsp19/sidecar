#ifndef SIDECAR_ALGORITHMS_OSCFAR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_OSCFAR_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Algorithm that performs ordered-statistics CFAR (OSCFAR) processing on Video message data. Performs
    thresholding of sample data by maintaining a sliding window of ordered samples and then using the N-th
    sample of the sliding window as the threshold value to use in the filter.
*/
class OSCFAR : public Algorithm {
public:
    enum InfoSlot { kWindowSize = ControllerStatus::kNumSlots, kThresholdIndex, kAlpha, kNumSlots };

    /** Constructor.

        \param controller the owner of the algorithm

        \param log the log device to use for OSCFAR log messages
    */
    OSCFAR(Controller& controller, Logger::Log& log);

    /** Override of Algorithm method. Registers Video message processor for input data, and the runtime
        parameters.

        \return true if successful
    */
    bool startup();

    /** Set the sliding window size of the algorithm.

        \param windowSize new window size to use
    */
    void setWindowSize(int windowSize) { windowSize_->setValue(windowSize); }

    /** Set the index of the window to use for threshold values. Must be >= 0 and < windowSize.

        \param thresholdIndex new threshold index to use
    */
    void setThresholdIndex(int thresholdIndex) { thresholdIndex_->setValue(thresholdIndex); }

    /** Scaling value for the filter threshold values. Multiplied with the sample value at the threshold index
        to create the threshold value.

        \param alpha new value to use
    */
    void setAlpha(double alpha) { alpha_->setValue(alpha); }

private:
    /** Obtain the number of info slots found in status messages from this algorithm.

        \return kNumSlots
    */
    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Override of Algorithm::setInfoSlots(). Stores XML representation of the two parameter values into the
        given XML-RPC container.

        \param status XML-RPC container to hold the stats
    */
    void setInfoSlots(IO::StatusBase& status);

    /** Message handler for Video messages.

        \param msg the incoming message to process

        \return true if successful
    */
    bool process(const Messages::Video::Ref& msg);

    Parameter::PositiveIntValue::Ref windowSize_;
    Parameter::PositiveIntValue::Ref thresholdIndex_;
    Parameter::DoubleValue::Ref alpha_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
