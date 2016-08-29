#ifndef SIDECAR_ALGORITHMS_ABTRACKER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_ABTRACKER_H

#include <list>

#include "Algorithms/Algorithm.h"
#include "Messages/Extraction.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {
namespace ABTrackerUtils { class Track; }

/** Documentation for the algorithm ABTracker. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class ABTracker : public Algorithm
{
    using Super = Algorithm;
public:

    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
	kAlpha,
	kBeta,
	kTrackCount,
	kTimeScaling,
        kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    ABTracker(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

    void setRotationDuration(double value)
	{ rotationDuration_->setValue(value); calculateDurations(); }

    double getRotationDuration() const
	{ return rotationDuration_->getValue(); }

    void setTimeScaling(double value)
	{ timeScaling_->setValue(value); calculateDurations(); }
    
    double getTimeScaling() const { return timeScaling_->getValue(); }

    void setAlpha(double value) { alpha_->setValue(value); }

    double getAlpha() const { return alpha_->getValue(); }

    void setBeta(double value) { beta_->setValue(value); }

    double getBeta() const { return beta_->getValue(); }

    void setAssociationRadius(double value)
	{ associationRadius_->setValue(value); }

    double getAssociationRadius() const
	{ return associationRadius_->getValue(); }

    void setInitiationCount(int value)
	{ initiationCount_->setValue(value); }

    uint32_t getInitiationCount() const { return initiationCount_->getValue(); }

    void setInitiationRotationCount(double value)
	{ initiationRotationCount_->setValue(value); calculateDurations(); }

    double getInitiationRotationCount() const
	{ return initiationRotationCount_->getValue(); }

    void setCoastRotationCount(double value)
	{ coastRotationCount_->setValue(value); calculateDurations(); }

    double getCoastRotationCount() const
	{ return coastRotationCount_->getValue(); }

    void setMinRange(double value)
	{ minRange_->setValue(value); }

    double getMinRange() const
	{ return minRange_->getValue(); }

    double getScaledMaxInitiationDuration() const
	{ return scaledMaxInitiationDuration_; }

    double getScaledMaxCoastDuration() const
	{ return scaledMaxCoastDuration_; }

private:

    bool updateTracks(const Messages::Extraction& plot);

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Extractions::Ref& msg);

    void resetTracker();

    void resetNotification(const Parameter::NotificationValue& value);

    void associationRadiusChanged(const Parameter::DoubleValue& value);

    void endParameterChanges();

    void calculateDurations();

    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::DoubleValue::Ref rotationDuration_;
    Parameter::DoubleValue::Ref timeScaling_;
    Parameter::DoubleValue::Ref alpha_;
    Parameter::DoubleValue::Ref beta_;
    Parameter::DoubleValue::Ref associationRadius_;
    Parameter::PositiveIntValue::Ref initiationCount_;
    Parameter::DoubleValue::Ref initiationRotationCount_;
    Parameter::DoubleValue::Ref coastRotationCount_;
    Parameter::DoubleValue::Ref minRange_;
    Parameter::NotificationValue::Ref reset_;

    double associationRadius2_;
    uint32_t trackIdGenerator_;
    using TrackList = std::list<ABTrackerUtils::Track*>;
    TrackList tracks_;
    size_t trackCount_;
    size_t initiatingCount_;
    size_t coastingCount_;

    struct Region {
	Region()
	    : name(), rangeMin(0.0), rangeMax(0.0),
	      azMin(0.0), azMax(0.0) {}
    	QString name;
	double rangeMin;
	double rangeMax;
	double azMin;
	double azMax;
    };

    using RegionVector = std::vector<Region>;
    RegionVector initiationInhibitRegions_;

    double scaledMaxInitiationDuration_;
    double scaledMaxCoastDuration_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
