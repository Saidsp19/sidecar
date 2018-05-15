#include "Settings.h"
#include "App.h"
#include "Configuration.h"
#include "FFTSettings.h"

using namespace SideCar::GUI::Spectrum;

Settings::Settings(QDoubleSpinBoxSetting* sampleFrequencyValue, QComboBoxSetting* sampleFrequencyScale,
                   ChannelSetting* inputChannel, ColorButtonSetting* qcolor, QCheckBoxSetting* showGrid,
                   QComboBoxSetting* drawingMode, QSpinBoxSetting* powerMax) :
    Super(),
    sampleFrequencyValue_(sampleFrequencyValue), sampleFrequencyScale_(sampleFrequencyScale),
    inputChannel_(inputChannel), qcolor_(qcolor), showGrid_(showGrid), drawingMode_(drawingMode),
    powerMax_(powerMax)
{
    add(sampleFrequencyValue);
    add(sampleFrequencyScale);
    add(inputChannel);
    add(showGrid);
    add(drawingMode);
    add(powerMax);

    connect(sampleFrequencyValue_, &QDoubleSpinBoxSetting::valueChanged, this, &Settings::updateSampleFrequency);
    connect(sampleFrequencyScale_, &QComboBoxSetting::valueChanged, this, &Settings::updateSampleFrequency);
    connect(qcolor_, &ColorButtonSetting::valueChanged, this, &Settings::handleColorChanged);
    connect(showGrid_, &QCheckBoxSetting::valueChanged, this, &Settings::showGridChanged);
    connect(powerMax_, &QSpinBoxSetting::valueChanged, this, &Settings::powerScalingChanged);
    connect(drawingMode_, &QComboBoxSetting::valueChanged, this, &Settings::drawingModeChanged);

    updateSampleFrequency();
}

void
Settings::updateSampleFrequency()
{
    sampleFrequencyScaling_ = ::pow(10, sampleFrequencyScale_->getValue() * 3);
    sampleFrequency_ = sampleFrequencyValue_->getValue() * sampleFrequencyScaling_;
    emit sampleFrequencyChanged(sampleFrequency_);
}

void
Settings::handleColorChanged(const QColor& color)
{
    color_ = Color(color.redF(), color.greenF(), color.blueF(), 1.0);
    emit colorChanged(color);
}
