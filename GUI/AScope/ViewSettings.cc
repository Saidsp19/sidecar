#include <cmath>

#include "QtCore/QSettings"

#include "Logger/Log.h"

#include "ViewSettings.h"

using namespace SideCar::GUI::AScope;

static const char* const kGateMin = "GateMin";
static const char* const kGateMax = "GateMax";
static const char* const kRangeMin = "RangeMin";
static const char* const kRangeMax = "RangeMax";
static const char* const kSampleMin = "SampleMin";
static const char* const kSampleMax = "SampleMax";
static const char* const kVoltageMin = "VoltageMin";
static const char* const kVoltageMax = "VoltageMax";
static const char* const kShowingRanges = "ShowingRanges";
static const char* const kShowingVoltages = "ShowingVoltages";

int const ViewSettings::kMetaTypeId =
    qRegisterMetaType<ViewSettings>("ViewSettings");

Logger::Log&
ViewSettings::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.ViewSettings");
    return log_;
}

ViewSettings::ViewSettings(double rangeMin, double rangeMax, int gateMin,
                           int gateMax, double voltageMin, double voltageMax,
                           int sampleMin, int sampleMax, bool showingRanges,
                           bool showingVoltages)
    : bounds_(), rangeMin_(rangeMin), rangeMax_(rangeMax),
      gateMin_(gateMin), gateMax_(gateMax), voltageMin_(voltageMin),
      voltageMax_(voltageMax), sampleMin_(sampleMin),
      sampleMax_(sampleMax), showingRanges_(showingRanges),
      showingVoltages_(showingVoltages)
{
    updateBounds();
}

void
ViewSettings::updateBounds()
{
    double xMin, xMax;
    if (showingRanges_) {
	xMin = rangeMin_;
	xMax = rangeMax_;
    }
    else {
	xMin = gateMin_;
	xMax = gateMax_;
    }

    double yMin, yMax;
    if (showingVoltages_) {
	yMin = voltageMin_;
	yMax = voltageMax_;
    }
    else {
	yMin = sampleMin_;
	yMax = sampleMax_;
    }

    bounds_ = ViewBounds(xMin, xMax, yMin, yMax);
}

ViewSettings::ViewSettings(QSettings& settings)
{
    restoreFromSettings(settings);
}

void
ViewSettings::restoreFromSettings(QSettings& settings)
{
    rangeMin_ = settings.value(kRangeMin).toDouble();
    rangeMax_ = settings.value(kRangeMax).toDouble();
    gateMin_ = settings.value(kGateMin).toInt();
    gateMax_ = settings.value(kGateMax).toInt();
    voltageMin_ = settings.value(kVoltageMin).toDouble();
    voltageMax_ = settings.value(kVoltageMax).toDouble();
    sampleMin_ = settings.value(kSampleMin).toInt();
    sampleMax_ = settings.value(kSampleMax).toInt();
    showingRanges_ = settings.value(kShowingRanges).toBool();
    showingVoltages_ = settings.value(kShowingVoltages).toBool();
    updateBounds();
}

void
ViewSettings::saveToSettings(QSettings& settings) const
{
    settings.setValue(kRangeMin, rangeMin_);
    settings.setValue(kRangeMax, rangeMax_);
    settings.setValue(kGateMin, gateMin_);
    settings.setValue(kGateMax, gateMax_);
    settings.setValue(kVoltageMin, voltageMin_);
    settings.setValue(kVoltageMax, voltageMax_);
    settings.setValue(kSampleMin, sampleMin_);
    settings.setValue(kSampleMax, sampleMax_);
    settings.setValue(kShowingRanges, showingRanges_);
    settings.setValue(kShowingVoltages, showingVoltages_);
}

void
ViewSettings::setBounds(const ViewBounds& bounds)
{
    Logger::ProcLog log("setBounds", Log());
    LOGINFO << "xMin: " << bounds.getXMin()
	    << " xMax: " << bounds.getXMax()
	    << " w: " << bounds.getWidth()
	    << "yMin: " << bounds.getYMin()
	    << "yMax: " << bounds.getYMax()
	    << " h: " << bounds.getHeight()
	    << std::endl;

    if (showingRanges_) {
	double span = rangeMax_ - rangeMin_;
	double dMin = (bounds.getXMin() - rangeMin_) / span;
	double dMax = (bounds.getXMax() - rangeMax_) / span;
	LOGDEBUG << "dMin: " << dMin << " dMax: " << dMax << std::endl;
	rangeMin_ = bounds.getXMin();
	rangeMax_ = bounds.getXMax();
	span = gateMax_ - gateMin_;
	gateMin_ += int(::rint(dMin * span));
	gateMax_ += int(::rint(dMax * span));
	LOGDEBUG << "gateMin: " << gateMin_ << " gateMax: " << gateMax_
		 << std::endl;
    }
    else {
	double span = gateMax_ - gateMin_;
	double dMin = (bounds.getXMin() - gateMin_) / span;
	double dMax = (bounds.getXMax() - gateMax_) / span;
	LOGDEBUG << "dMin: " << dMin << " dMax: " << dMax << std::endl;
	gateMin_ = int(::rint(bounds.getXMin()));
	gateMax_ = int(::rint(bounds.getXMax()));
	span = rangeMax_ - rangeMin_;
	rangeMin_ += dMin * span;
	rangeMax_ += dMax * span;
	LOGDEBUG << "rangeMin: " << rangeMin_ << " rangeMax: " << rangeMax_
		 << std::endl;
    }

    if (showingVoltages_) {
	double span = voltageMax_ - voltageMin_;
	double dMin = (bounds.getYMin() - voltageMin_) / span;
	double dMax = (bounds.getYMax() - voltageMax_) / span;
	LOGDEBUG << "dMin: " << dMin << " dMax: " << dMax << std::endl;
	voltageMin_ = bounds.getYMin();
	voltageMax_ = bounds.getYMax();
	span = sampleMax_ - sampleMin_;
	sampleMin_ += int(::rint(dMin * span));
	sampleMax_ += int(::rint(dMax * span));
	LOGDEBUG << "sampleMin: " << sampleMin_ << " sampleMax: " << sampleMax_
		 << std::endl;
    }
    else {
	double span = sampleMax_ - sampleMin_;
	double dMin = (bounds.getYMin() - sampleMin_) / span;
	double dMax = (bounds.getYMax() - sampleMax_) / span;
	LOGDEBUG << "dMin: " << dMin << " dMax: " << dMax << std::endl;
	sampleMin_ = int(::rint(bounds.getYMin()));
	sampleMax_ = int(::rint(bounds.getYMax()));
	span = voltageMax_ - voltageMin_;
	voltageMin_ += dMin * span;
	voltageMax_ += dMax * span;
	LOGDEBUG << "voltageMin: " << voltageMin_ << " voltageMax: "
		 << voltageMax_ << std::endl;
    }

    bounds_ = bounds;
}

void
ViewSettings::setShowingRanges(bool state)
{
    if (showingRanges_ != state) {
	showingRanges_ = state;
	updateBounds();
    }
}
	
void
ViewSettings::setShowingVoltages(bool state)
{
    if (showingVoltages_ != state) {
	showingVoltages_ = state;
	updateBounds();
    }
}

bool
ViewSettings::operator==(const ViewSettings& rhs) const
{
    return rangeMin_ == rhs.rangeMin_ &&
	rangeMax_ == rhs.rangeMax_ &&
	gateMin_ == rhs.gateMin_ &&
	gateMax_ == rhs.gateMax_ &&
	voltageMin_ == rhs.voltageMin_ &&
	voltageMax_ == rhs.voltageMax_ &&
	sampleMin_ == rhs.sampleMin_ &&
	sampleMax_ == rhs.sampleMax_ &&
	showingRanges_ == rhs.showingRanges_ &&
	showingVoltages_ == rhs.showingVoltages_;
}
