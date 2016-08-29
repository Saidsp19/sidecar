#include "QtCore/QSettings"

#include "GUI/IntSetting.h"
#include "GUI/LogUtils.h"
#include "GUI/QSliderSetting.h"
#include "Utils/Utils.h"

#include "ViewSettings.h"

static const char* const kRangeFactor = "RangeFactor";
static const char* const kRangeMaxMax = "RangeMaxMax";

using namespace SideCar::GUI::BScope;

Logger::Log&
ViewSettings::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.ViewSettings");
    return log_;
}

ViewSettings::ViewSettings(IntSetting* azimuthZero, IntSetting* azimuthSpan,
                           DoubleSetting* rangeMin, DoubleSetting* rangeMax)
    : Super(), azimuthZero_(azimuthZero), azimuthSpan_(azimuthSpan),
      rangeMin_(rangeMin), rangeMax_(rangeMax), rangeFactor_(-1.0),
      rangeMaxMax_(-1.0)
{
    azimuthZero_->setEnabled(true);
    restore();
    add(rangeMin);
    add(rangeMax);
    connect(azimuthZero_, SIGNAL(valueChanged(int)),
            SLOT(recalculateAzimuthMinMax()));
    connect(azimuthSpan_, SIGNAL(valueChanged(int)),
            SLOT(recalculateAzimuthMinMax()));
    recalculateAzimuthMinMax();
}

void
ViewSettings::recalculateAzimuthMinMax()
{
    Logger::ProcLog log("recalculateAzimuthMinMax", Log());
    double zero = Utils::degreesToRadians(double(azimuthZero_->getValue()));
    LOGINFO << "zero: " << azimuthZero_->getValue()
	    << " span: " << azimuthSpan_->getValue() << std::endl;

    azimuthMin_ = -zero;

    azimuthMax_ = Utils::degreesToRadians(azimuthSpan_->getValue() -
                                          azimuthZero_->getValue());

    LOGDEBUG << "azimuthMin: " << azimuthMin_ << ' '
	     << Utils::radiansToDegrees(azimuthMin_)
	     << " azimuthMax: " << azimuthMax_ << ' '
	     << Utils::radiansToDegrees(azimuthMax_) << std::endl;

    postSettingChanged();
}

double
ViewSettings::normalizedAzimuth(double radians) const
{
    static Logger::ProcLog log("normalizedAzimuth", Log());
    LOGINFO << "input: " << radians << ' ' << Utils::radiansToDegrees(radians)
	    << std::endl;
    
    if (radians >= azimuthMax_) {
	double tmp = radians - Utils::kCircleRadians;
	if (tmp >= azimuthMin_)
	    radians = tmp;
    }
    else if (radians < azimuthMin_) {
	double tmp = radians + Utils::kCircleRadians;
	if (tmp < azimuthMax_)
	    radians = tmp;
    }

    LOGINFO << "output: " << radians << ' '
	    << Utils::radiansToDegrees(radians) << std::endl;

    return radians;
}

double
ViewSettings::azimuthToParametric(double radians) const
{
    return (radians - azimuthMin_) / (azimuthMax_ - azimuthMin_) ;
}

double
ViewSettings::azimuthFromParametric(double value) const
{
    return Utils::normalizeRadians(value * (azimuthMax_ - azimuthMin_) +
                                   azimuthMin_);
}

double
ViewSettings::rangeToParametric(double value) const
{
    return (value - rangeMin_->getValue()) /
	(rangeMax_->getValue() - rangeMin_->getValue());
}

double
ViewSettings::rangeFromParametric(double value) const
{
    return value * (rangeMax_->getValue() - rangeMin_->getValue()) +
	rangeMin_->getValue();
}

void
ViewSettings::setRangeFactorAndMax(double rangeFactor, double rangeMaxMax)
{
    rangeFactor_ = rangeFactor;
    rangeMaxMax_ = rangeMaxMax;
    save();
}

void
ViewSettings::restore()
{
    QSettings settings;
    rangeFactor_ = settings.value(kRangeFactor).toDouble();
    rangeMaxMax_ = settings.value(kRangeMaxMax).toDouble();
}

void
ViewSettings::save()
{
    QSettings settings;
    settings.setValue(kRangeFactor, rangeFactor_);
    settings.setValue(kRangeMaxMax, rangeMaxMax_);
}

bool
ViewSettings::viewingAzimuth(double radians) const
{
    radians = normalizedAzimuth(radians);
    if (radians >= azimuthMin_ && radians < azimuthMax_) return true;
    return false;
}
