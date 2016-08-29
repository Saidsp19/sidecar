#ifndef SIDECAR_ALGORITHMS_MOFN_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_MOFN_H

#include "Algorithms/Algorithm.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** This algorithm performs "M of N" (binary integration) processing on a series of input PRIs. The algorithm
    takes in PRIs of binary values (detections). For a given range cell, the output will be high if M of the N
    range cells in a window surrounding the cell are high. The window width and length are defined by the
    runtime parameters numPRIs and numGates. The runtime parameter threshold is a double between 0 and 1 that
    specifies the fraction of range gates in the window that need to be high in order to trigger an output
    detection.
*/

class MofN : public Algorithm
{
public:

    using DetectionCountType = uint16_t;
    using DetectionCountVector = std::vector<DetectionCountType>;

    /** Internal class that contains data retained between PRI message processing. Holds the binary video PRI
	message and the detected counts from the PRI message.
    */
    struct RetainedEntry {
	Messages::BinaryVideo::Ref video;
	DetectionCountVector detectionCounts;
    };

    using RetainedEntryVector = std::vector<RetainedEntry>;

    enum InfoSlot {
	kNumPRIs = ControllerStatus::kNumSlots,
	kNumGates,
	kThreshold,
	kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    MofN(Controller& controller, Logger::Log& log);

    /** Registers runtime parameters with the controller. Override of the Algorithm::startup() method.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Reset the algorithm to a known state. Purges retained PRI messages and calculated counts. Override of
        Algorithm::reset().

        \return always true
    */
    bool reset();

private:

    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Override of Algorithm::setInfoSlots(). Stores XML representation of cancellation statistics ino the
        given XML-RPC container.

        \param status XML-RPC container to hold the stats
    */
    void setInfoSlots(IO::StatusBase& status);

    struct ThresholdRange {
	static double GetMinValue() { return 0.0; }
	static double GetMaxValue() { return 1.0; }
    };

    using ThresholdDef = Parameter::Defs::RangedDouble<ThresholdRange>;
    using Threshold = Parameter::TValue<ThresholdDef>;

    /** Process the next BinaryVideo message, and possibly emit a fitered BinaryVideo message.

        \param in BinaryVideo message to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::BinaryVideo::Ref& in);

    /** Notification handler called when the numPRIs_ parameter value changes. Invokes reset() to forget
        calculated values based on the old numPRIs_ value.

        \param value Parameter object that signalled.
    */
    void numPRIsChanged(const Parameter::PositiveIntValue& value);

    /** Notification handler called when the numGates_ parameter value changes. Invokes reset() to forget
        calculated values based on the old numGates_ value.

        \param value Parameter object that signalled.
    */
    void numGatesChanged(const Parameter::PositiveIntValue& value);

    /** Notification handler called when the threshold_ parameter value changes. Does nothing, since held values
        do not depend on the threshold value.

        \param value Parameter object that signalled.
    */
    void thresholdChanged(const Threshold& value);

    void calculateThresholdValue();

    /** Value of the detection counts found for numPRIs_ messages.
     */
    DetectionCountVector runningCounts_;

    /** Data retained between PRI messages.
     */
    RetainedEntryVector retained_;
    size_t retainedCount_;
    size_t oldestIndex_;
    size_t newestIndex_;

    Parameter::BoolValue::Ref enabled_;

    /** Runtime parameter window width. This is the number of PRI messages needed before a calculation can be
	done.
    */
    Parameter::PositiveIntValue::Ref numPRIs_;

    /** Runtime parameter window length. This is the number of gates in a PRI message that are used for count
	summation.
    */
    Parameter::PositiveIntValue::Ref numGates_;

    /** Runtime parameter threshold. This is the fractional number of cells in the MxN window which must be
	true. Valid values lie between 0 and 1, inclusive.
    */
    Threshold::Ref threshold_;

    uint16_t thresholdValue_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
