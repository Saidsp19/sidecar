#ifndef SIDECAR_ALGORITHMS_CLUTTER_MAP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CLUTTER_MAP_H

#include "boost/scoped_ptr.hpp"

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include "MapBuffer.h"

namespace SideCar {
namespace Algorithms {

/** Clutter map filter. Creates the clutter map from the first \c learningScanCount scans worth of PRI samples,
    at which point the map is frozen and subsequent PRIs update it through a low-pass filter (single pole IIR)
    controlled by the \c alpha parameter.

    The map consists of a collection of radial buffers, of size \c radialPartitionCount. The PRI's shaft
    encoding value dictates which radial buffer to use. All map manipulations happen inside the MapBuffer class.

    \subsection Run-time Parameters

    - \c alpha non-negative real value that determines how much of an effect new PRIs have on a frozen clutter
    map. The MapBuffer class uese a single-pole lowpass filter parameter to update a radial buffer from a frozen
    clutter map:

    \code
    map[i] = (1 - alpha) * map[i] + alpha * PRI
    \endcode

    where \c i is the partition index calculated from the PRI shaft encoding
    value.

    - \c learningScanCount specifies how many complete radar scans / rotations to use when building the clutter
    map. After initial startup or reset, the algorithm will ignore PRIs until it detects a zero azimuth (north)
    crossing, at which point it will apply incoming PRIs to the map buffer.

    - \c radialPartitionCount specifies the numter of radials in the map buffer. Larger values improve azimuthal
    resolution in the map at the expense of memory, and possibly learning time if the resolution is such that
    some radials fail to receive an update during one scan due to rotation speed and accuracy of the shaft
    encoder.

    - \c loadFilePath path of file containg the values of a previously-saved clutter map. A loaded map is
    automatically frozen.

    - \c saveFilePath path of a file to hold the values of clutter map. If set, it does not have any effect
    until the clutter map becomes frozen.

    - \c resetBuffer notification to clear the clutter map and enter the map building phase.

    \subsection Status Information

    ClutterMap overrides the getInfoData() to return the following attributes:

    - \c learning boolean value that is false when the map is frozen, true otherwise.

    - \c remaining contains the number of scans remainging before the map transitions from learning to frozen
    state.
*/
class ClutterMap : public Algorithm {
public:
    enum InfoSlot { kLearning = ControllerStatus::kNumSlots, kRemaining, kEnabled, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    ClutterMap(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    bool reset();

    /** Assign a new value to the low-pass filter alpha parameter. Must be non-negative.

        \param value new value to use
    */
    void setAlpha(double value) { alpha_->setValue(value); }

    /** Set the number of radar scans to use to build the clutter map.

        \param value new value to use
    */
    void setLearningScanCount(int value) { learningScanCount_->setValue(value); }

    /** Set the number of radials in the map buffer.

        \param value new value to use
    */
    void setRadialPartitionCount(int value) { radialPartitionCount_->setValue(value); }

    /** Load a previously-saved clutter map from a system file.

        \param path location of the file
    */
    void setLoadFilePath(const std::string& path) { loadFilePath_->setValue(path); }

    /** Save a frozen clutter map to a system file.

        \param path location of the file
    */
    void setSaveFilePath(const std::string& path) { saveFilePath_->setValue(path); }

    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Implementation of Algorithm::getInfoData() interface.

        \param infoData XML structure to store into

        \return the string "ClutterMap"
    */
    void setInfoSlots(IO::StatusBase& status);

    /** Load a built clutter map from a system file.

        \param path locate of the file to read from
    */
    bool loadMap(const std::string& path);

    /** Save a built clutter map to a system file.

        \param path location of the file to write to
    */
    bool saveMap(const std::string& path);

private:
    /** Implementation of the Algorithm::process interface.

        \param mgr object containing the encoded or native data to process

        \return true if successful, false otherwise
    */
    bool process(const Messages::Video::Ref& msg);

    void alphaChanged(const Parameter::DoubleValue& value);

    void radialPartitionCountChanged(const Parameter::PositiveIntValue& value);

    void loadFilePathChanged(const Parameter::ReadPathValue& value);

    void saveFilePathChanged(const Parameter::WritePathValue& value);

    void resetBufferNotification(const Parameter::NotificationValue& param);

    void loadMapNotification(const Parameter::NotificationValue& param);

    void saveMapNotification(const Parameter::NotificationValue& param);

    void resetBuffer();

    Parameter::BoolValue::Ref enabled_;
    Parameter::BoolValue::Ref beginInLearningMode_;
    Parameter::DoubleValue::Ref alpha_;
    Parameter::PositiveIntValue::Ref learningScanCount_;
    Parameter::PositiveIntValue::Ref radialPartitionCount_;
    Parameter::ReadPathValue::Ref loadFilePath_;
    Parameter::NotificationValue::Ref loadMap_;
    Parameter::WritePathValue::Ref saveFilePath_;
    Parameter::NotificationValue::Ref saveMap_;
    Parameter::NotificationValue::Ref resetBuffer_;

    size_t lastShaftEncoding_;
    int scanCounter_;
    boost::scoped_ptr<MapBuffer> mapBuffer_;
};

} // namespace Algorithms
} // namespace SideCar

#endif
