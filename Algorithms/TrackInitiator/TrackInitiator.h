#ifndef SIDECAR_ALGORITHMS_TRACK_INITIATOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_TRACK_INITIATOR_H

#include <cmath>
#include <list>
#include <time.h> // for time_t and friends
#include <vector>

#include "Algorithms/Algorithm.h"
#include "Messages/Extraction.h"
#include "Messages/Track.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms
*/
class TrackInitiator : public Algorithm {
public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    TrackInitiator(Controller& controller, Logger::Log& log);

    /** Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool startup();

    bool reset();

private:
    void init();

    /**

       \param mgr object containing the encoded or native data to process

       \return true if successful, false otherwise
    */
    bool process(const Messages::Extractions::Ref& msg);

    using Extraction = SideCar::Messages::Extraction;

    struct Data {
        Data(const Messages::Extraction& extraction) :
            when_(extraction.getWhen().asDouble()), extraction_(extraction), velocity_()
        {
        }
        double when_;
        Messages::Extraction extraction_;
        Messages::Track::Coord velocity_;
    };

    using Entry = std::list<Data>;
    std::vector<std::vector<Entry>> buffer_; // [x][y]
    size_t indexOffset_;

    void corrCell(Data&, Entry& candidates);
    void corr(Data&);

    double rMax_;
    size_t numBins_;

    Parameter::DoubleValue::Ref param_searchRadius;

    void on_searchRadius_changed(const Parameter::DoubleValue&);

    Parameter::IntValue::Ref param_scanTime;

    void on_scanTime_changed(const Parameter::IntValue&);

    Parameter::IntValue::Ref param_numScans;

    float searchRadius_;  // the maximum gating size for correlation to occur
    float searchRadius2_; // = searchRadius^2

    Parameter::DoubleValue::Ref param_assumedAltitude;
    Parameter::DoubleValue::Ref param_minRange;

    double t_new_, t_old_, t_veryold_;

    int currentTrackNum_; // ELY,G
};

} // namespace Algorithms
} // namespace SideCar

/** \file
 */

#endif
