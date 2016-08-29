#include <fstream>

#include "boost/bind.hpp"

#include "Logger/Log.h"
#include "Messages/RadarConfig.h"

#include "ClutterMap.h"
#include "ClutterMap_defaults.h"
#include "MapBuffer.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

using VideoT = SideCar::Messages::Video::DatumType;

ClutterMap::ClutterMap(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled",
                                          kDefaultEnabled)),
      beginInLearningMode_(Parameter::BoolValue::Make("beginInLearningMode", "Begin in Learning Mode",
                                                      kDefaultBeginInLearningMode)),
      learningScanCount_(Parameter::PositiveIntValue::Make("learningScanCount", "Learning Scan Count",
                                                           kDefaultLearningScanCount)),
      radialPartitionCount_(Parameter::PositiveIntValue::Make("radialPartitionCount", "Radial Partition Count",
                                                              kDefaultRadialPartitionCount)),
      alpha_(Parameter::DoubleValue::Make("alpha", "Learn Rate", kDefaultAlpha)),
      loadFilePath_(Parameter::ReadPathValue::Make("loadFilePath", "Load File", kDefaultLoadFilePath)),
      loadMap_(Parameter::NotificationValue::Make("loadMap", "Load Map", 0)),
      saveFilePath_(Parameter::WritePathValue::Make("saveFilePath", "Save File", kDefaultSaveFilePath)),
      saveMap_(Parameter::NotificationValue::Make("saveMap", "Save Map", 0)),
      resetBuffer_(Parameter::NotificationValue::Make("resetBuffer", "Reset Map Buffer", 0)),
      lastShaftEncoding_(0), scanCounter_(-1), mapBuffer_()
{
    alpha_->connectChangedSignalTo(boost::bind(&ClutterMap::alphaChanged, this, _1));
    radialPartitionCount_->connectChangedSignalTo(boost::bind(&ClutterMap::radialPartitionCountChanged, this, _1));
    resetBuffer_->connectChangedSignalTo(boost::bind(&ClutterMap::resetBufferNotification, this, _1));
    loadMap_->connectChangedSignalTo(boost::bind(&ClutterMap::loadMapNotification, this, _1));
    saveMap_->connectChangedSignalTo(boost::bind(&ClutterMap::saveMapNotification, this, _1));
}

bool
ClutterMap::startup()
{
    Logger::ProcLog log("startup", getLog());
    LOGERROR << std::endl;

    registerProcessor<ClutterMap,Messages::Video>(&ClutterMap::process);
    return registerParameter(enabled_) &&
	registerParameter(beginInLearningMode_) &&
	registerParameter(learningScanCount_) &&
	registerParameter(radialPartitionCount_) &&
	registerParameter(alpha_) &&
	registerParameter(loadFilePath_) &&
	registerParameter(loadMap_) &&
	registerParameter(saveFilePath_) &&
	registerParameter(saveMap_) &&
	registerParameter(resetBuffer_) &&
	Algorithm::startup();
}

bool
ClutterMap::reset()
{
    // The algorithm is transitioning from a stop state to a run state. If we are configured to load a saved map and we
    // don't have one, do so now.
    //
    if (! mapBuffer_ && ! beginInLearningMode_->getValue() && loadFilePath_->getValue().size() > 0) {
	if (loadMap(loadFilePath_->getValue())) {
	    beginInLearningMode_->setValue(true);
	}
    }

    return true;
}

void
ClutterMap::resetBuffer()
{
    Logger::ProcLog log("resetBuffer", getLog());
    LOGERROR << std::endl;
    scanCounter_ = 0;
    lastShaftEncoding_ = 0;
    mapBuffer_.reset();
    beginInLearningMode_->setValue(true);
}

bool
ClutterMap::process(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processVideo", getLog());
    LOGINFO << std::endl;

    // Special case to detect very first PRI seen so we can then detect north (0 azimuth) crossings.
    //
    size_t shaftEncoding = msg->getShaftEncoding();
    LOGDEBUG << "shaftEncoding: " << shaftEncoding << std::endl;

    if (scanCounter_ == -1) {
	scanCounter_ = 0;
	lastShaftEncoding_ = shaftEncoding;
	return send(msg);
    }

    // Detect north crossing.
    //
    static const size_t kShaftEncodingHysteresis = 100;

    if (shaftEncoding + kShaftEncodingHysteresis < lastShaftEncoding_) {
	LOGINFO << "north mark crossing - scanCounter: " << scanCounter_
		<< " limit: " << learningScanCount_->getValue()
		<< std::endl;

	if (! mapBuffer_) {
	    mapBuffer_.reset(new MapBuffer(getName(), radialPartitionCount_->getValue(), alpha_->getValue()));
	}
	else if (! mapBuffer_->isFrozen()) {
	    if (++scanCounter_ == learningScanCount_->getValue()) {
		LOGWARNING << "freezing clutter map" << std::endl;
		mapBuffer_->freeze();
		std::string path = saveFilePath_->getValue();
		if (path.size()) {
		    if (! saveMap(path)) {
			LOGERROR << "failed to save frozen map" << std::endl;
		    }
		}
	    }
	}
    }

    lastShaftEncoding_ = shaftEncoding;
    if (! enabled_->getValue() || ! mapBuffer_) {
	return send(msg);
    }

    Messages::Video::Ref out = mapBuffer_->add(msg);
    bool rc = send(out);

    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

void
ClutterMap::alphaChanged(const Parameter::DoubleValue& value)
{
    Logger::ProcLog log("alphaChanged", getLog());
    if (mapBuffer_) {
	mapBuffer_->setAlpha(value.getValue());
    }
}

void
ClutterMap::radialPartitionCountChanged(const Parameter::PositiveIntValue& value)
{
    Logger::ProcLog log("radialPartitionCountChanged", getLog());
    LOGERROR << std::endl;
    resetBuffer();
}

void
ClutterMap::resetBufferNotification(const Parameter::NotificationValue& value)
{
    Logger::ProcLog log("resetBufferNotification", getLog());
    LOGERROR << std::endl;
    resetBuffer();
}

void
ClutterMap::loadMapNotification(const Parameter::NotificationValue& value)
{
    Logger::ProcLog log("loadMapNotification", getLog());
    LOGERROR << std::endl;
    std::string path = loadFilePath_->getValue();
    if (path.size() > 0) {
	loadMap(path);
    }
}

void
ClutterMap::saveMapNotification(const Parameter::NotificationValue& value)
{
    Logger::ProcLog log("saveMapNotification", getLog());
    LOGERROR << std::endl;
    std::string path = saveFilePath_->getValue();
    if (path.size() > 0 && mapBuffer_ && mapBuffer_->isFrozen()) {
	saveMap(path);
    }
}

void
ClutterMap::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());

    if (mapBuffer_ && mapBuffer_->isFrozen()) {
	status.setSlot(kLearning, false);
	status.setSlot(kRemaining, 0);
    }
    else {
	status.setSlot(kLearning, true);
	status.setSlot(kRemaining, learningScanCount_->getValue() - scanCounter_);
    }
}

bool
ClutterMap::loadMap(const std::string& path)
{
    Logger::ProcLog log("loadMap", getLog());

    std::ifstream is(path.c_str());
    if (! is) {
	LOGERROR << "failed to open map file '" << path << "'" << std::endl;
	return false;
    }

    mapBuffer_.reset(new MapBuffer(getName(), radialPartitionCount_->getValue(), alpha_->getValue()));
    if (! mapBuffer_->load(is)) {
	LOGERROR << "failed to load map '" << path << "'" << std::endl;
	return false;
    }

    return true;
}

bool
ClutterMap::saveMap(const std::string& path)
{
    Logger::ProcLog log("saveMap", getLog());

    std::ofstream os(path.c_str());
    if (! os) {
	LOGERROR << "failed to open map file '" << path << "'" << std::endl;
	return false;
    }

    if (! mapBuffer_->save(os)) {
	LOGERROR << "failed to write to map '" << path << "'" << std::endl;
	return false;
    }

    return true;
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;

    bool enabled = status[ClutterMap::kEnabled];
    if (! enabled) return Algorithm::FormatInfoValue("Disabled");

    bool learning = status[ClutterMap::kLearning];
    if (! learning) return Algorithm::FormatInfoValue("Active");
    
    int remaining = status[ClutterMap::kRemaining];
    return Algorithm::FormatInfoValue(QString("Building (%1 scan%2 remaining)")
                                      .arg(remaining)
                                      .arg(remaining == 1 ? "" : "s"));
}

// Dynamic library function for creating a new ClutterMap object.
//
extern "C" ACE_Svc_Export Algorithm*
ClutterMapMake(Controller& controller, Logger::Log& log)
{
    return new ClutterMap(controller, log);
}
