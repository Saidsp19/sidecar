#include "App.h"
#include "Configuration.h"
#include "FFTSettings.h"
#include "Settings.h"

using namespace SideCar::GUI::Spectrum;

Settings::Settings(QDoubleSpinBoxSetting* sampleFrequencyValue,
                   QComboBoxSetting* sampleFrequencyScale,
                   ChannelSetting* inputChannel, ColorButtonSetting* qcolor,
                   QCheckBoxSetting* showGrid, QComboBoxSetting* drawingMode,
                   QSpinBoxSetting* powerMax)
    : Super(), sampleFrequencyValue_(sampleFrequencyValue),
      sampleFrequencyScale_(sampleFrequencyScale),
      inputChannel_(inputChannel), qcolor_(qcolor),
      showGrid_(showGrid), drawingMode_(drawingMode), powerMax_(powerMax)
{
    add(sampleFrequencyValue);
    add(sampleFrequencyScale);
    add(inputChannel);
    add(showGrid);
    add(drawingMode);
    add(powerMax);

    connect(sampleFrequencyValue_, SIGNAL(valueChanged(double)),
            SLOT(updateSampleFrequency()));
    connect(sampleFrequencyScale_, SIGNAL(valueChanged(int)),
            SLOT(updateSampleFrequency()));
    connect(qcolor_, SIGNAL(valueChanged(const QColor&)),
            SLOT(handleColorChanged(const QColor&)));
    connect(showGrid_, SIGNAL(valueChanged(bool)),
            SIGNAL(showGridChanged(bool)));
    connect(powerMax_, SIGNAL(valueChanged(int)),
            SIGNAL(powerScalingChanged()));
    connect(drawingMode_, SIGNAL(valueChanged(int)),
            SIGNAL(drawingModeChanged()));

    updateSampleFrequency();
}

void
Settings::updateSampleFrequency()
{
    sampleFrequencyScaling_ =
	::pow(10, sampleFrequencyScale_->getValue() * 3);
    sampleFrequency_ = sampleFrequencyValue_->getValue() *
	sampleFrequencyScaling_;
    emit sampleFrequencyChanged(sampleFrequency_);
}

void
Settings::handleColorChanged(const QColor& color)
{
    color_ = Color(color.redF(), color.greenF(), color.blueF(), 1.0);
    emit colorChanged(color);
}
