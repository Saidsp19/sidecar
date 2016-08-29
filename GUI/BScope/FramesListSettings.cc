#include "FramesListSettings.h"

using namespace SideCar::GUI::BScope;

FramesListSettings::FramesListSettings(QSpinBoxSetting* scalingPower)
    : Super(), scalingPower_(scalingPower)
{
    add(scalingPower);
    connect(scalingPower, SIGNAL(valueChanged(int)),
            SLOT(changeScale(int)));
}

void
FramesListSettings::changeScale(int power)
{
    emit scaleChanged(GetScale(power));
}

void
FramesListSettings::changeScalingPower(int change)
{
    int newValue = scalingPower_->getValue() + change;
    if (newValue < -5) newValue = -5;
    else if (newValue > 5) newValue = 5;
    if (newValue != scalingPower_->getValue()) {
	scalingPower_->setValue(newValue);
    }
}
