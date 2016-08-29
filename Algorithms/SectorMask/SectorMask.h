#ifndef SIDECAR_ALGORITHMS_SECTORMASK_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SECTORMASK_H

#include "Algorithms/Algorithm.h"
#include "Messages/BinaryVideo.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** 
    This algorithm masks out any hits within a given sector of the observable area. The "sector" area is
    specified with a min/max azimuth and range. Any detections in a Binary message within this sector are
    removed.
  
*/
class SectorMask : public Algorithm
{
    using Super = Algorithm;
public:

    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
        kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    SectorMask(Controller& controller, Logger::Log& log);

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

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::BinaryVideo::Ref& msg);
    /** Normalize and check inputs the user provides based on current values and the message size.
     */
    bool NormalizeBins(int& startRngBin, int& endRngBin, size_t msg_size);

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;

    // Parameters for specifying the sector to be masked. Azimuth is given in degrees and range is given in
    // bins.
    // 
    Parameter::DoubleValue::Ref minAzimuth_;
    Parameter::DoubleValue::Ref maxAzimuth_;
    Parameter::IntValue::Ref minRangeBin_;
    Parameter::IntValue::Ref maxRangeBin_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
