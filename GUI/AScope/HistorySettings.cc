#include "HistorySettings.h"

using namespace SideCar::GUI::AScope;

HistorySettings::HistorySettings(BoolSetting* enabled, IntSetting* duration)
    : Super(enabled), duration_(duration)
{
    add(duration);
    connect(duration, SIGNAL(valueChanged(int)),
            SIGNAL(durationChanged(int)));
}
