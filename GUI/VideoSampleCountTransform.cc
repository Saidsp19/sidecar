#include "GUI/LogUtils.h"

#include "VideoSampleCountTransform.h"

using namespace SideCar::GUI;

const double VideoSampleCountTransform::kLogEpsilon = 1.0E-10;

Logger::Log&
VideoSampleCountTransform::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.VideoSampleCountTransform");
    return log_;
}

VideoSampleCountTransform::VideoSampleCountTransform(
    IntSetting* sampleMin, IntSetting* sampleMax,
    IntSetting* gain, IntSetting* cutoffMin, IntSetting* cutoffMax,
    BoolSetting* showDecibels)
    : Super(), sampleMin_(sampleMin), sampleMax_(sampleMax),
      cutoffMin_(cutoffMin), cutoffMax_(cutoffMax),
      showDecibels_(showDecibels)
{
    add(sampleMin);
    add(sampleMax);
    add(cutoffMin);
    add(cutoffMax);
    add(gain);
    add(showDecibels);
    
    connect(gain, SIGNAL(valueChanged(int)), SLOT(gainChanged(int)));
    connect(showDecibels, SIGNAL(valueChanged(bool)),
            SLOT(showDecibelsChanged(bool)));

    gainChanged(gain->getValue());
    recalculate();
}

void
VideoSampleCountTransform::emitSettingChanged()
{
    static Logger::ProcLog log("emitSettingChanged", Log());
    LOGINFO << std::endl;
    recalculate();
    Super::emitSettingChanged();
}

void
VideoSampleCountTransform::gainChanged(int gain)
{
    gainValue_ = 1.0 + gain * 0.01;
}

void
VideoSampleCountTransform::showDecibelsChanged(bool value)
{
    recalculate();
    emit showingDecibels(value);
}

void
VideoSampleCountTransform::recalculate()
{
    static Logger::ProcLog log("recalculate", Log());

    int sampleMin = sampleMin_->getValue();
    int sampleMax = sampleMax_->getValue();

    if (sampleMin > cutoffMax_->getValue() ||
        sampleMax < cutoffMax_->getValue()) {
	cutoffMax_->setValue(sampleMax);
    }

    if (sampleMin > cutoffMin_->getValue() ||
        sampleMax < cutoffMin_->getValue()) {
	cutoffMin_->setValue(sampleMin);
    }

    thresholdMin_ = cutoffMin_->getValue();
    thresholdMax_ = cutoffMax_-> getValue();

    LOGINFO << "min: " << thresholdMin_ << " max: " << thresholdMax_
	    << std::endl;

    if (showDecibels_->getValue()) {
	offset_ = - thresholdMin_ + 1;
	scale_ = 1.0 / (thresholdMax_ - thresholdMin_ + 1);
	minLog_ = decibel(thresholdMin_);
	maxLog_ = decibel(thresholdMax_);
	normalizer_ = 1.0 / (maxLog_ - minLog_);
    }
    else {

	// We want a linear transformation from [thresholdMin,thresholdMax] to [0.0,1.0]. This should be OUT = (
	//   IN - thresholdMin) / (thresholdMax - thresholdMin)
	//
	offset_ = -thresholdMin_;
	normalizer_ = 1.0 / (thresholdMax_ - thresholdMin_);
    }
}

double
VideoSampleCountTransform::transform(int sampleCount) const
{
    // Apply any gain setting and clamp result to [threshold-sampleMax]
    //
    double value = sampleCount * gainValue_;
    if (value < thresholdMin_)
	value = thresholdMin_;
    else if (value > thresholdMax_)
	value = thresholdMax_;

    if (showDecibels_->getValue())
	value = (decibel(value) - minLog_) * normalizer_;
    else
	value = (value + offset_) * normalizer_;

    return value;
}

int
VideoSampleCountTransform::inverseTransform(double value) const
{
    if (showDecibels_->getValue()) {
	value /= normalizer_;
	value += minLog_;
	value /= 20.0;
	value = ::pow(10, value);
	value /= scale_;
	value -= offset_;
	return int(::rint(value));
    }
    else {
	return int(::rint(value / normalizer_ - offset_));
    }
}
