#include <cmath>

#include "FFTSettings.h"
#include "Settings.h"

using namespace SideCar::GUI::Spectrum;

FFTSettings::FFTSettings(QComboBoxSetting* fftSizePower, QSpinBoxSetting* gateStart, QComboBoxSetting* windowType,
                         QCheckBoxSetting* zeroPad, QSpinBoxSetting* workerThreadCount,
                         QSpinBoxSetting* smoothing, Settings* settings) :
    Super(), fftSizePower_(fftSizePower), gateStart_(gateStart), windowType_(windowType), zeroPad_(zeroPad),
    workerThreadCount_(workerThreadCount), smoothing_(smoothing)
{
    add(fftSizePower_);
    add(gateStart_);
    add(windowType_);
    add(zeroPad_);
    add(workerThreadCount_);
    add(smoothing_);

    connect(fftSizePower_, &QComboBoxSetting::valueChanged, this, &FFTSettings::updateFFTSize);
    connect(gateStart_, &QSpinBoxSetting::valueChanged, this, &FFTSettings::gateStartChanged);
    connect(windowType_, &QComboBoxSetting::valueChanged, this, &FFTSettings::windowTypeChanged);
    connect(zeroPad_, &QCheckBoxSetting::valueChanged, this, &FFTSettings::zeroPadChanged);
    connect(workerThreadCount_, &QSpinBoxSetting::valueChanged, this, &FFTSettings::workerThreadCountChanged);
    connect(smoothing_, &QSpinBoxSetting::valueChanged, this, &FFTSettings::inputSmoothingChanged);
    connect(settings, &Settings::sampleFrequencyChanged, this, &FFTSettings::sampleFrequencyChanged);

    sampleFrequencyChanged(settings->getSampleFrequency());

    updateFFTSize();
}

void
FFTSettings::updateFFTSize()
{
    fftSize_ = int(::pow(2, fftSizePower_->getValue() + 7));
    frequencyStep_ = -2.0 * frequencyMin_ / fftSize_;
    emit fftSizeChanged(fftSize_);
}

void
FFTSettings::sampleFrequencyChanged(double sampleFrequency)
{
    double nyquist = sampleFrequency / 2.0;
    frequencyMin_ = -nyquist;
    frequencyStep_ = sampleFrequency / double(fftSize_);
    emit frequencyStepChanged(frequencyStep_);
}
