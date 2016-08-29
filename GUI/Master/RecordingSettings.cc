#include "RecordingSettings.h"

using namespace SideCar::GUI::Master;

RecordingSettings::RecordingSettings(BoolSetting* durationEnabled,
                                     TimeSetting* duration)
    : Super(), durationEnabled_(durationEnabled), duration_(duration)
{
    add(durationEnabled);
    add(duration);
}

void
RecordingSettings::connectWidgets(QCheckBox* durationEnabled,
                                  QTimeEdit* duration)
{
    durationEnabled_->connectWidget(durationEnabled);
    duration_->connectWidget(duration);
}
