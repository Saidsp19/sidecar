#include <cmath>

#include "FFTSettings.h"
#include "Settings.h"

using namespace SideCar::GUI::Spectrum;

FFTSettings::FFTSettings(QComboBoxSetting* fftSizePower, QSpinBoxSetting* gateStart, QComboBoxSetting* windowType,
                         QCheckBoxSetting* zeroPad, QSpinBoxSetting* workerThreadCount, QSpinBoxSetting* smoothing,
                         Settings* settings) :
    Super(),
    fftSizePower_(fftSizePower), gateStart_(gateStart), windowType_(windowType), zeroPad_(zeroPad),
    workerThreadCount_(workerThreadCount), smoothing_(smoothing)
{
    add(fftSizePower_);
    add(gateStart_);
    add(windowType_);
    add(zeroPad_);
    add(workerThreadCount_);
    add(smoothing_);

    connect(fftSizePower_, SIGNAL(valueChanged(int)), SLOT(updateFFTSize()));
    connect(gateStart_, SIGNAL(valueChanged(int)), SIGNAL(gateStartChanged(int)));
    connect(windowType_, SIGNAL(valueChanged(int)), SIGNAL(windowTypeChanged(int)));
    connect(zeroPad_, SIGNAL(valueChanged(bool)), SIGNAL(zeroPadChanged(bool)));
    connect(workerThreadCount_, SIGNAL(valueChanged(int)), SIGNAL(workerThreadCountChanged(int)));
    connect(smoothing_, SIGNAL(valueChanged(int)), SIGNAL(inputSmoothingChanged(int)));
    connect(settings, SIGNAL(sampleFrequencyChanged(double)), SLOT(sampleFrequencyChanged(double)));

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
