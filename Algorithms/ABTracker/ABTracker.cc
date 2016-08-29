#include "QtCore/QString"

#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Algorithms/Utils.h"
#include "Logger/Log.h"
#include "Messages/TSPI.h"

#include "ABTracker.h"
#include "ABTracker_defaults.h"

#include "Track.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::ABTrackerUtils;

ABTracker::ABTracker(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      rotationDuration_(Parameter::DoubleValue::Make("rotationDuration", "Radar Rotation Duration (s)",
                                                     kDefaultRotationDuration)),
      timeScaling_(Parameter::DoubleValue::Make("timeScaling", "Rotation Time Scaling", kDefaultTimeScaling)),
      alpha_(Parameter::DoubleValue::Make("alpha", "Alpha", kDefaultAlpha)),
      beta_(Parameter::DoubleValue::Make("beta", "Beta", kDefaultBeta)),
      associationRadius_(Parameter::DoubleValue::Make("associationRadius", "Track Association Radius",
                                                      kDefaultAssociationRadius)),
      initiationCount_(Parameter::PositiveIntValue::Make("initiationCount", "Track Initiation Count",
                                                         kDefaultInitiationCount)),
      initiationRotationCount_(Parameter::DoubleValue::Make("initiationRotationCount",
                                                            "Max Track Initiation Rotation Count",
                                                            kDefaultInitiationRotationCount)),
      coastRotationCount_(Parameter::DoubleValue::Make("coastRotationCount", "Max Track Coast Rotations",
                                                       kDefaultCoastRotationCount)),
      minRange_ (Parameter::DoubleValue::Make("minRange", "Minimum Range", kDefaultMinRange)),
      reset_(Parameter::NotificationValue::Make("reset", "Reset", 0)), trackIdGenerator_(), tracks_(),
      trackCount_(0), initiatingCount_(0), coastingCount_(0)
{
    associationRadius2_ = associationRadius_->getValue();
    associationRadius2_ *= associationRadius2_;
    reset_->connectChangedSignalTo(boost::bind(&ABTracker::resetNotification, this, _1));
    associationRadius_->connectChangedSignalTo(boost::bind(&ABTracker::associationRadiusChanged, this, _1));
}

bool
ABTracker::startup()
{
    endParameterChanges();
    registerProcessor<ABTracker,Messages::Extractions>(&ABTracker::processInput);
    return Super::startup() &&
	registerParameter(enabled_) &&
	registerParameter(rotationDuration_) &&
	registerParameter(timeScaling_) &&
	registerParameter(alpha_) &&
	registerParameter(beta_) &&
	registerParameter(associationRadius_) &&
	registerParameter(initiationCount_) &&
	registerParameter(initiationRotationCount_) &&
	registerParameter(coastRotationCount_) &&
	registerParameter(minRange_) &&
	registerParameter(reset_);
}

bool
ABTracker::shutdown()
{
    resetTracker();
    return Super::shutdown();
}

void
ABTracker::resetTracker()
{
    while (trackCount_) {
	Track* track = tracks_.back();
	track->drop();
	delete track;
	tracks_.pop_back();
	--trackCount_;
    }

    initiatingCount_ = 0;
    coastingCount_ = 0;
}

bool
ABTracker::processInput(const Messages::Extractions::Ref& msg)
{
    Logger::ProcLog log("processInput", getLog());
    LOGINFO << std::endl;

    // For each Extraction report in the message call updateTracks.
    //
    bool ok = true;
    for (size_t index = 0; index < msg->size(); ++index) {
	if (! updateTracks(msg[index]))
	    ok = false;
    }

    return ok;
}

bool
ABTracker::updateTracks(const Messages::Extraction& plot)
{
    Logger::ProcLog log("updateTracks", getLog());
    double when = plot.getWhen().asDouble();

    LOGINFO << "when: " << when
	    << " range: " << plot.getRange()
	    << " az: " << plot.getAzimuth()
	    << std::endl;

    Track* found = 0;
    if (plot.getRange() >= minRange_->getValue()) {

	// We only associate with something if it has a proximity value smaller
	// than associationRadius. We use the squared variants to save us from
	// the costly square root.
	//
	double best = associationRadius2_;
	Geometry::Vector v(plot.getX(), plot.getY(), plot.getElevation());

	// Visit all of the existing Track objects to find the one that best
	// associates with the given Extraction plot.
	//
	TrackList::iterator pos = tracks_.begin();
	TrackList::iterator end = tracks_.end();
	while (pos != end) {
	    Track* track = *pos++;
	    double distance = track->getProximityTo(when, v);
	    if (distance < best) {
		found = track;
		best = distance;
	    }
	}

	// None found, so create a new Track object that starts in the
	// kInitiating state.
	//
	if (! found) {
	    found = new Track(*this, ++trackIdGenerator_, when , v);
	    LOGINFO << "created new track - " << found->getId() << std::endl;
	    tracks_.push_back(found);
	    ++trackCount_;
	}
	else {
	    LOGINFO << "found existing track - " << found->getId() << std::endl;

	    // Apply the Extraction plot to the found track
	    //
	    found->updatePosition(when, v);
	}
    }

    // Revisit all of the tracks, emitting TSPI messages for those that are
    // dropping and the one that was updated above.
    //
    bool ok = true;
    TrackList::iterator pos = tracks_.begin();
    TrackList::iterator end = tracks_.end();
    while (pos != end) {
	Track* track = *pos;
	if (track == found || track->isDropping()) {
	    if (! track->emitPosition())
		ok = false;
	    if (track->isDropping()) {
		LOGINFO << "dropping track " << track->getId() << std::endl;
		delete track;
		pos = tracks_.erase(pos);
		--trackCount_;
		continue;
	    }
	}
	++pos;
    }

    return ok;
}

void
ABTracker::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kAlpha, alpha_->getValue());
    status.setSlot(kBeta, beta_->getValue());
    status.setSlot(kTrackCount, int(trackCount_));
}

void
ABTracker::resetNotification(const Parameter::NotificationValue& value)
{
    resetTracker();
}

void
ABTracker::associationRadiusChanged(const Parameter::DoubleValue& value)
{
    associationRadius2_ = value.getValue();
    associationRadius2_ *= associationRadius2_;
}

void
ABTracker::endParameterChanges()
{
    calculateDurations();
}

void
ABTracker::calculateDurations()
{
    double scaledRotationDuration = rotationDuration_->getValue() * timeScaling_->getValue();
    scaledMaxInitiationDuration_ = initiationRotationCount_->getValue() * scaledRotationDuration;
    scaledMaxCoastDuration_ = coastRotationCount_->getValue() * scaledRotationDuration;
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[ABTracker::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return Algorithm::FormatInfoValue(QString("Tracks: %1  Alpha: %2  Beta: %3")
                                      .arg(int(status[ABTracker::kTrackCount]))
                                      .arg(double(status[ABTracker::kAlpha]))
                                      .arg(double(status[ABTracker::kBeta])));
}

// Factory function for the DLL that will create a new instance of the ABTracker class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
ABTrackerMake(Controller& controller, Logger::Log& log)
{
    return new ABTracker(controller, log);
}
