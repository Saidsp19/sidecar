#include "PeakBar.h"
#include "PeakBarSettings.h"

using namespace SideCar::GUI::AScope;

PeakBarSettings::PeakBarSettings(BoolSetting* enabled, IntSetting* width,
                                 IntSetting* lifeTime, BoolSetting* fading)
    : Super(enabled), width_(width->getValue()),
      lifeTime_(lifeTime->getValue()), fading_(fading->getValue())
{
    PeakBar::SetLifeTime(lifeTime_);

    add(width);
    add(lifeTime);
    add(fading);

    connect(width, SIGNAL(valueChanged(int)),
            SLOT(widthChange(int)));
    connect(lifeTime, SIGNAL(valueChanged(int)),
            SLOT(lifeTimeChange(int)));
    connect(fading, SIGNAL(valueChanged(bool)),
            SLOT(fadingChange(bool)));
}

void
PeakBarSettings::widthChange(int value)
{
    width_ = value;
    emit widthChanged(value);
}

void
PeakBarSettings::lifeTimeChange(int value)
{
    lifeTime_ = value;
    PeakBar::SetLifeTime(value);
    emit lifeTimeChanged(value);
}

void
PeakBarSettings::fadingChange(bool value)
{
    fading_ = value;
    emit fadingChanged(value);
}

