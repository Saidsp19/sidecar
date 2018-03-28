#include "PlayerSettings.h"

using namespace SideCar::GUI::BScope;

PlayerSettings::PlayerSettings(QComboBoxSetting* playbackRate, QSpinBoxSetting* scalingPower,
                               QCheckBoxSetting* looping) :
    Super(),
    playbackRate_(playbackRate), scalingPower_(scalingPower), looping_(looping)
{
    add(playbackRate);
    add(scalingPower);
    add(looping);

    connect(playbackRate, SIGNAL(valueChanged(int)), SLOT(changePlaybackRate(int)));
    connect(scalingPower, SIGNAL(valueChanged(int)), SLOT(changeScale(int)));
}

void
PlayerSettings::changePlaybackRate(int index)
{
    emit playbackRateChanged(GetPlaybackRate(index));
}

void
PlayerSettings::changeScale(int power)
{
    emit scaleChanged(GetScale(power));
}

void
PlayerSettings::changeScalingPower(int change)
{
    int newValue = scalingPower_->getValue() + change;
    if (newValue < -5)
        newValue = -5;
    else if (newValue > 5)
        newValue = 5;
    if (newValue != scalingPower_->getValue()) { scalingPower_->setValue(newValue); }
}

void
PlayerSettings::connectPlayer(QComboBox* playbackRate, QCheckBox* looping)
{
    playbackRate_->connectWidget(playbackRate);
    looping_->connectWidget(looping);
}
