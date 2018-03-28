#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/Track.h"
#include "Utils/Utils.h"

#include "ComposeTracks.h"
#include "ComposeTracks_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Algorithms;

ComposeTracks::ComposeTracks(Controller& controller, Logger::Log& log) :
    Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled))
{
    num_alarms_ = 0;
}

bool
ComposeTracks::startup()
{
    setAlarm(30);

    return registerParameter(enabled_) && Super::startup();
}

void
ComposeTracks::processAlarm()
{
    static Logger::ProcLog log("my_handler", getLog());

    // create new Track message with current estimated position specified
    Messages::Track::Ref trk = Track::Make("ComposeTracks");
    std::vector<double> pos_estimate;
    pos_estimate.resize(3);
    pos_estimate[GEO_LAT] = Utils::degreesToRadians(37.837453);
    pos_estimate[GEO_LON] = Utils::degreesToRadians(-116.554105);
    trk->setEstimate(pos_estimate);

    trk->setTrackNumber(0);

    switch (num_alarms_) {
    case 0: make_new_track(trk); break;
    case 1: make_corrected_track(trk, 10); break;
    case 2: make_corrected_track(trk, 20); break;
    case 3: make_corrected_track(trk, 30); break;
    case 4: make_corrected_track(trk, 40); break;
    case 5: make_corrected_track(trk, 50); break;
    }

    if (num_alarms_ <= 5) {
        bool code = send(trk);

        LOGERROR << "ComposeTracks::sent track message with error code " << code << std::endl;

        LOGERROR << trk->printHeader(std::cerr) << std::endl;

        LOGERROR << trk->printData(std::cerr) << std::endl;
    }

    num_alarms_++;
}

void
ComposeTracks::make_needs_prediction(Messages::Track::Ref trk)
{
    // This code can be used to test the predict function of StateEstimator module

    trk->setFlags(Track::kNeedsPrediction);

    // set prediction time needed and velocity estimate
    trk->setPredictionTime(20.0);
    std::vector<double> velocity_llh;
    velocity_llh.resize(3);

    velocity_llh[GEO_LAT] = Utils::degreesToRadians(0.00010000);
    velocity_llh[GEO_LON] = Utils::degreesToRadians(0.00113015);
    trk->setVelocity(velocity_llh);
}

void
ComposeTracks::make_needs_correction(Messages::Track::Ref trk)
{
    // this code can be used to test the correct function of StateEstimator module

    trk->setFlags(Track::kNeedsCorrection);

    // set prediction
    trk->setPredictionTime(20.0);
    std::vector<double> prediction_llh;
    prediction_llh.resize(3);
    prediction_llh[GEO_LAT] = Utils::degreesToRadians(37.8395);
    prediction_llh[GEO_LON] = Utils::degreesToRadians(-116.532);
    trk->setPrediction(prediction_llh);

    // set extraction
    trk->setExtractionTime(20.0);
    std::vector<double> extraction_rae;
    extraction_rae.resize(3);

    extraction_rae[GEO_AZ] = Utils::degreesToRadians(5.0);
    extraction_rae[GEO_EL] = 0;
    extraction_rae[GEO_RNG] = 2000;
    trk->setExtraction(extraction_rae);
}

void
ComposeTracks::make_new_track(Messages::Track::Ref trk)
{
    trk->setFlags(Track::kNew);

    // set velocity estimate
    std::vector<double> velocity_llh;
    velocity_llh.resize(3);

    velocity_llh[GEO_LAT] = Utils::degreesToRadians(0.00010000);
    velocity_llh[GEO_LON] = Utils::degreesToRadians(0.00113015);
    trk->setVelocity(velocity_llh);

    // set extraction time & pos
    trk->setExtractionTime(0);
    std::vector<double> extraction_rae;
    extraction_rae.resize(3);

    extraction_rae[GEO_AZ] = Utils::degreesToRadians(5.0);
    extraction_rae[GEO_EL] = 0;
    extraction_rae[GEO_RNG] = 2000;
    trk->setExtraction(extraction_rae);

    trk->setType(Track::kTentative);
}

void
ComposeTracks::make_corrected_track(Messages::Track::Ref trk, double corrected_time)
{
    trk->setFlags(Track::kCorrected);

    // set last prediction time & pos
    trk->setPredictionTime(corrected_time);
    std::vector<double> prediction_llh;
    prediction_llh.resize(3);
    prediction_llh[GEO_LAT] = Utils::degreesToRadians(37.8395);
    prediction_llh[GEO_LON] = Utils::degreesToRadians(-116.532);
    trk->setPrediction(prediction_llh);

    // set extraction time & pos
    trk->setExtractionTime(corrected_time);
    std::vector<double> extraction_rae;
    extraction_rae.resize(3);

    extraction_rae[GEO_AZ] = Utils::degreesToRadians(5.0);
    extraction_rae[GEO_EL] = 0;
    extraction_rae[GEO_RNG] = 2000;
    trk->setExtraction(extraction_rae);

    // set position and velocity estimate & time
    trk->setWhen(corrected_time);

    std::vector<double> velocity_llh;
    velocity_llh.resize(3);
    velocity_llh[GEO_LAT] = Utils::degreesToRadians(0.00010000);
    velocity_llh[GEO_LON] = Utils::degreesToRadians(0.00113015);
    trk->setVelocity(velocity_llh);

    trk->setType(Track::kTentative);
}

bool
ComposeTracks::shutdown()
{
    return Super::shutdown();
}

void
ComposeTracks::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[ComposeTrackes::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the ABTracker class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
ComposeTracksMake(Controller& controller, Logger::Log& log)
{
    return new ComposeTracks(controller, log);
}
