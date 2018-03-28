#ifndef SIDECAR_ALGORITHMS_COMPOSETRACKS_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_COMPOSETRACKS_H

#include <list>

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm ProcessTracks This task simply receives track messages and passes them
    along.
*/
class ComposeTracks : public Algorithm {
    using Super = Algorithm;

public:
    enum InfoSlots { kEnabled = ControllerStatus::kNumSlots, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    ComposeTracks(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

    /** This method will get called whenever an alarm goes off for this Algorithm
     */
    void processAlarm();

    void make_needs_prediction(Messages::Track::Ref trk);

    void make_needs_correction(Messages::Track::Ref trk);

    void make_new_track(Messages::Track::Ref trk);

    void make_corrected_track(Messages::Track::Ref trk, double corrected_time);

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;

    int num_alarms_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
