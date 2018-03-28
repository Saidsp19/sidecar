#ifndef SIDECAR_GUI_ESSCOPE_RADARSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_RADARSETTINGS_H

#include "GUI/BoolSetting.h"
#include "GUI/DoubleSetting.h"
#include "GUI/IntSetting.h"
#include "GUI/SettingsBlock.h"
#include "GUI/Vertex.h"
#include "Messages/PRIMessage.h"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {

namespace ESScope {

class RadarSettings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    static Logger::Log& Log();

    RadarSettings(IntSetting* alphaScans, IntSetting* betaScans, IntSetting* rangeScans, DoubleSetting* alphaMinMin,
                  DoubleSetting* alphaMaxMax, DoubleSetting* betaMinMin, DoubleSetting* betaMaxMax,
                  DoubleSetting* rangeMinMin, DoubleSetting* rangeMaxMax, DoubleSetting* sampleRangeMin,
                  DoubleSetting* sampleRangeFactor, DoubleSetting* tilt, DoubleSetting* rotation,
                  IntSetting* firstSample, IntSetting* lastSample, BoolSetting* allynHack,
                  BoolSetting* ignoreMessageRangeSettings);

    int getAlphaScans() const { return alphaScans_->getValue(); }

    int getBetaScans() const { return betaScans_->getValue(); }

    int getRangeScans() const { return rangeScans_->getValue(); }

    double getAlphaMinMin() const { return alphaMinMin_->getValue(); }

    double getAlphaMaxMax() const { return alphaMaxMax_->getValue(); }

    double getAlphaSpan() const { return getAlphaMaxMax() - getAlphaMinMin(); }

    double getBetaMinMin() const { return betaMinMin_->getValue(); }

    double getBetaMaxMax() const { return betaMaxMax_->getValue(); }

    double getBetaSpan() const { return getBetaMaxMax() - getBetaMinMin(); }

    double getRangeMinMin() const { return rangeMinMin_->getValue(); }

    double getRangeMaxMax() const { return rangeMaxMax_->getValue(); }

    double getRangeSpan() const { return getRangeMaxMax() - getRangeMinMin(); }

    double getRangeMin() const { return rangeMin_; }

    double getRangeFactor() const { return rangeFactor_; }

    double getRange(int sample) const { return rangeFactor_ * sample + rangeMin_; }

    int getRangeIndex(double range) const;

    int getSampleRangeIndex(int sampleIndex) const { return getRangeIndex(getRange(sampleIndex)); }

    double getTilt() const { return tilt_->getValue(); }

    double getRotation() const { return rotation_->getValue(); }

    int getFirstSample() const { return firstSample_->getValue(); }

    int getLastSample() const { return lastSample_->getValue(); }

    void getAzimuthElevation(double alpha, double beta, double* azimuth, double* elevation) const;

    void getAlphaBeta(double azimuth, double elevation, double* alpha, double* beta) const;

    double getAlpha(const Messages::PRIMessage::Ref& msg) const;

    int getAlphaIndex(const Messages::PRIMessage::Ref& msg) const;

    double getAlpha(int alphaIndex) const;

    int getAlphaIndex(double alpha) const;

    double getBeta(const Messages::PRIMessage::Ref& msg) const;

    int getBetaIndex(const Messages::PRIMessage::Ref& msg) const;

    double getBeta(int betaIndex) const;

    int getBetaIndex(double beta) const;

    void setRangeScaling(double rangeMin, double rangeFactor);

signals:

    void alphaMinMaxChanged(double alphaMin, double alphaMax);

    void betaMinMaxChanged(double betaMin, double betaMax);

    void rangeMinMaxChanged(double rangeMin, double rangeMax);

    void scansChanged(int alphaScans, int betaScans, int rangeScans);

    void tiltChanged(double value);

    void rotationChanged(double value);

    void rangeScalingChanged();

private slots:

    void updateSinesCosines();

    void dimensionChanged();

    void alphaChanged();

    void betaChanged();

    void rangeChanged();

    void updateSampleRangeSettings();

    void changeMessageRangeSettings(bool ignoreMessageRangeSettings);

private:
    IntSetting* alphaScans_;
    IntSetting* betaScans_;
    IntSetting* rangeScans_;
    DoubleSetting* alphaMinMin_;
    DoubleSetting* alphaMaxMax_;
    DoubleSetting* betaMinMin_;
    DoubleSetting* betaMaxMax_;
    DoubleSetting* rangeMinMin_;
    DoubleSetting* rangeMaxMax_;
    DoubleSetting* sampleRangeMin_;
    DoubleSetting* sampleRangeFactor_;
    DoubleSetting* tilt_;
    DoubleSetting* rotation_;
    IntSetting* firstSample_;
    IntSetting* lastSample_;
    BoolSetting* allynHack_;
    BoolSetting* ignoreMessageRangeSettings_;
    double cosineTilt_;
    double sineTilt_;
    double cosineRotation_;
    double sineRotation_;
    double sineRotationSineTilt_;
    double sineRotationCosineTilt_;
    double cosineRotationSineTilt_;
    double cosineRotationCosineTilt_;
    double rangeMin_;
    double rangeFactor_;
    double messageRangeMin_;
    double messageRangeFactor_;
};

} // namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
