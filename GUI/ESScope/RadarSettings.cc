#include <cmath>

#include "GUI/BoolSetting.h"
#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "RadarSettings.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

Logger::Log&
RadarSettings::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.ViewSettings");
    return log_;
}

RadarSettings::RadarSettings(IntSetting* alphaScans, IntSetting* betaScans,
                             IntSetting* rangeScans,
                             DoubleSetting* alphaMinMin,
                             DoubleSetting* alphaMaxMax,
                             DoubleSetting* betaMinMin,
                             DoubleSetting* betaMaxMax,
                             DoubleSetting* rangeMinMin,
                             DoubleSetting* rangeMaxMax,
                             DoubleSetting* sampleRangeMin,
                             DoubleSetting* sampleRangeFactor,
                             DoubleSetting* tilt, DoubleSetting* rotation,
                             IntSetting* firstSample, IntSetting* lastSample,
                             BoolSetting* allynHack,
                             BoolSetting* ignoreMessageRangeSettings)
: Super(), alphaScans_(alphaScans), betaScans_(betaScans),
  rangeScans_(rangeScans), alphaMinMin_(alphaMinMin),
  alphaMaxMax_(alphaMaxMax), betaMinMin_(betaMinMin),
  betaMaxMax_(betaMaxMax), rangeMinMin_(rangeMinMin),
  rangeMaxMax_(rangeMaxMax), sampleRangeMin_(sampleRangeMin),
  sampleRangeFactor_(sampleRangeFactor), tilt_(tilt),
  rotation_(rotation), firstSample_(firstSample),
  lastSample_(lastSample), allynHack_(allynHack),
  ignoreMessageRangeSettings_(ignoreMessageRangeSettings),
  rangeMin_(0.0), rangeFactor_(0.0), messageRangeMin_(0.0),
  messageRangeFactor_(0.0)
{
    add(alphaScans);
    add(betaScans);
    add(rangeScans);
    add(alphaMinMin);
    add(alphaMaxMax);
    add(betaMinMin);
    add(betaMaxMax);
    add(rangeMinMin);
    add(rangeMaxMax);
    add(sampleRangeMin);
    add(sampleRangeFactor);
    add(tilt);
    add(rotation);
    add(firstSample);
    add(lastSample);
    add(ignoreMessageRangeSettings);

    connect(alphaScans, SIGNAL(valueChanged(int)),
            SLOT(dimensionChanged()));
    connect(betaScans, SIGNAL(valueChanged(int)),
            SLOT(dimensionChanged()));
    connect(rangeScans, SIGNAL(valueChanged(int)),
            SLOT(dimensionChanged()));
    connect(alphaMinMin, SIGNAL(valueChanged(double)),
            SLOT(alphaChanged()));
    connect(alphaMaxMax, SIGNAL(valueChanged(double)),
            SLOT(alphaChanged()));
    connect(betaMinMin, SIGNAL(valueChanged(double)),
            SLOT(betaChanged()));
    connect(betaMaxMax, SIGNAL(valueChanged(double)),
            SLOT(betaChanged()));
    connect(rangeMinMin, SIGNAL(valueChanged(double)),
            SLOT(rangeChanged()));
    connect(rangeMaxMax, SIGNAL(valueChanged(double)),
            SLOT(rangeChanged()));
    connect(sampleRangeMin, SIGNAL(valueChanged(double)),
            SLOT(updateSampleRangeSettings()));
    connect(sampleRangeFactor, SIGNAL(valueChanged(double)),
            SLOT(updateSampleRangeSettings()));
    connect(tilt, SIGNAL(valueChanged(double)),
            SLOT(updateSinesCosines()));
    connect(tilt, SIGNAL(valueChanged(double)),
            SIGNAL(tiltChanged(double)));
    connect(rotation_, SIGNAL(valueChanged(double)),
            SLOT(updateSinesCosines()));
    connect(rotation_, SIGNAL(valueChanged(double)),
            SIGNAL(rotationChanged(double)));
    connect(ignoreMessageRangeSettings, SIGNAL(valueChanged(bool)),
            SLOT(changeMessageRangeSettings(bool)));

    changeMessageRangeSettings(ignoreMessageRangeSettings->getValue());

    updateSinesCosines();
}

void
RadarSettings::updateSinesCosines()
{
    double tilt = Utils::degreesToRadians(tilt_->getValue());
    cosineTilt_ = ::cos(tilt);
    sineTilt_ = ::sin(tilt);
    double rotation = Utils::degreesToRadians(rotation_->getValue());
    cosineRotation_ = ::cos(rotation);
    sineRotation_ = ::sin(rotation);

    sineRotationSineTilt_ = sineRotation_ * sineTilt_;
    sineRotationCosineTilt_ = sineRotation_ * cosineTilt_;
    cosineRotationSineTilt_ = cosineRotation_ * sineTilt_;
    cosineRotationCosineTilt_ = cosineRotation_ * cosineTilt_;
}

void
RadarSettings::getAzimuthElevation(double alpha, double beta,
                                   double* azimuth, double* elevation) const
{
    double sqrt1a2b2 = ::sqrt(1.0 - alpha * alpha - beta * beta);
    *elevation = ::asin(beta * cosineTilt_ + sqrt1a2b2 * sineTilt_);
    *azimuth = ::atan2(alpha * cosineRotation_ -
                       beta * sineRotationSineTilt_ +
                       sqrt1a2b2 * sineRotationCosineTilt_,
                       -alpha * sineRotation_
                       - beta * cosineRotationSineTilt_ +
                       sqrt1a2b2 * cosineRotationCosineTilt_);
}

void
RadarSettings::getAlphaBeta(double azimuth, double elevation, double* alpha,
                            double* beta) const
{
    double cosineElevation = ::cos(elevation);
    double sineElevation = ::sin(elevation);
    double cosineAzimuth = ::cos(azimuth);
    double sineAzimuth = ::sin(azimuth);
    *alpha = cosineElevation * sineAzimuth * cosineRotation_ -
	cosineElevation * cosineAzimuth * sineRotation_;
    *beta = -cosineElevation * sineAzimuth * sineRotationSineTilt_ +
	sineElevation * cosineTilt_ - cosineElevation * cosineAzimuth *
	cosineRotationSineTilt_;
}

void
RadarSettings::dimensionChanged()
{
    emit scansChanged(alphaScans_->getValue(), betaScans_->getValue(),
                      rangeScans_->getValue());
}

double
RadarSettings::getAlpha(const Messages::PRIMessage::Ref& msg) const
{
    if (allynHack_->getValue()) {

	// Convert shaft encoding value into value from [alphaMin, alphaMax]
	//
	double alpha = double(msg->getRIUInfo().shaftEncoding) /
	    double(Messages::RadarConfig::GetShaftEncodingMax()) - 0.5;
	if (alpha > 0.0)
	    return alpha - 0.5;
	else if (alpha < 0.0)
	    return alpha + 0.5;
	return alpha;
    }

    unsigned int alphaInt = ((msg->getRIUInfo().prfEncoding >> 17) & 0x7FFF);
    double N = (alphaInt * 360.0) / 32768.0;
    if (N > 180.0) N -= 360.0;
    return N / (0.6835 * 360.0);
}

int
RadarSettings::getAlphaIndex(const Messages::PRIMessage::Ref& msg) const
{
    return getAlphaIndex(getAlpha(msg));
}

double
RadarSettings::getAlpha(int alphaIndex) const
{
    return double(alphaIndex) / double(getAlphaScans()) *
	getAlphaSpan() + getAlphaMinMin();
}

int
RadarSettings::getAlphaIndex(double alpha) const
{
    // Obtain normalized value from [0.0, 1.0]
    //
    alpha = (alpha - getAlphaMinMin()) / getAlphaSpan();

    // Scale normalized alpha into a scan index from [0, alphaScans - 1]. There will be multiple alphas for one
    // scan line as the radar either dwells in the same azimuth, or it steps thru the range of beta values.
    //
    int index = int(::rint(alpha * (getAlphaScans() - 1)));
    if (index < 0) index = 0;
    if (index >= getAlphaScans()) index = getAlphaScans() - 1;
    return index;
}

double
RadarSettings::getBeta(const Messages::PRIMessage::Ref& msg) const
{
    if (allynHack_->getValue())
	return msg->getRIUInfo().irigTime;

    unsigned int betaInt = ((msg->getRIUInfo().prfEncoding >> 1) & 0x7FFFF);
    double M = (betaInt * 360.0) / 32768.0;
    return M / (0.3927 * 360.0);
}

int
RadarSettings::getBetaIndex(const Messages::PRIMessage::Ref& msg) const
{
    return getBetaIndex(getBeta(msg));
}

double
RadarSettings::getBeta(int betaIndex) const
{
    return double(betaIndex) / double(getBetaScans()) *
	getBetaSpan() + getBetaMinMin();
}

int
RadarSettings::getBetaIndex(double beta) const
{
    // Obtain normalized value from [0.0, 1.0]
    //
    beta = (beta - getBetaMinMin()) / getBetaSpan();

    // Scale normalized beta into a scan index from [0, betaScans - 1].
    //
    int index = int(::rint(beta * (getBetaScans() - 1)));
    if (index < 0) index = 0;
    if (index >= getBetaScans()) index = getBetaScans() - 1;
    return index;
}

void
RadarSettings::alphaChanged()
{
    emit alphaMinMaxChanged(alphaMinMin_->getValue(),
                            alphaMaxMax_->getValue());
}

void
RadarSettings::betaChanged()
{
    emit betaMinMaxChanged(betaMinMin_->getValue(),
                           betaMaxMax_->getValue());
}

void
RadarSettings::rangeChanged()
{
    emit rangeMinMaxChanged(rangeMinMin_->getValue(),
                            rangeMaxMax_->getValue());
}

void
RadarSettings::setRangeScaling(double rangeMin, double rangeFactor)
{
    if (messageRangeMin_ != rangeMin || messageRangeFactor_ != rangeFactor) {
	messageRangeMin_ = rangeMin;
	messageRangeFactor_ = rangeFactor;
	if (! ignoreMessageRangeSettings_->getValue()) {
	    rangeMin_ = messageRangeMin_;
	    rangeFactor_ = messageRangeFactor_;
	    sampleRangeMin_->setValue(rangeMin);
	    sampleRangeFactor_->setValue(rangeFactor);
	    emit rangeScalingChanged();
	}
    }
}

int
RadarSettings::getRangeIndex(double range) const
{
    range = (range - getRangeMinMin()) / getRangeSpan();
    int index = int(::rint(range * (getRangeScans() - 1)));
    if (index < 0) index = 0;
    if (index >= getRangeScans()) index = getRangeScans() - 1;
    return index;
}

void
RadarSettings::updateSampleRangeSettings()
{
    if (ignoreMessageRangeSettings_->getValue()) {
	rangeMin_ = sampleRangeMin_->getValue();
	rangeFactor_ = sampleRangeFactor_->getValue();
	emit rangeScalingChanged();
    }
}

void
RadarSettings::changeMessageRangeSettings(bool ignoreMessageSettings)
{
    if (ignoreMessageSettings) {
	rangeMin_ = sampleRangeMin_->getValue();
	rangeFactor_ = sampleRangeFactor_->getValue();
    }
    else {
	rangeMin_ = messageRangeMin_;
	rangeFactor_ = messageRangeFactor_;
    }

    emit rangeScalingChanged();
}
