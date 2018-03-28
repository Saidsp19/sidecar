#include "DurationSetting.h"

using namespace SideCar::GUI::Master;

static int
convertTimeToDuration(const QTime& time)
{
    return time.hour() * 3600 + time.minute() * 60 + time.second();
}

DurationSetting::DurationSetting(PresetManager* mgr, QTimeEdit* widget, bool global) :
    Super(mgr, widget, global), duration_(convertTimeToDuration(widget->time()))
{
    ;
}

void
DurationSetting::valueUpdated()
{
    duration_ = convertTimeToDuration(Super::getValue());
    Super::valueUpdated();
}
