#include "HistorySettings.h"

using namespace SideCar::GUI::PPIDisplay;

HistorySettings::HistorySettings(BoolSetting* enabled, IntSetting* retentionSize) :
    Super(enabled), retentionSize_(retentionSize)
{
    connect(retentionSize, SIGNAL(valueChanged(int)), SIGNAL(retentionSizeChanged(int)));
}
