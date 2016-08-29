#ifndef SIDECAR_GUI_MASTER_RECORDINGSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_RECORDINGSETTINGS_H

#include "GUI/BoolSetting.h"
#include "GUI/SettingsBlock.h"
#include "GUI/TimeSetting.h"

namespace SideCar {
namespace GUI {
namespace Master {

class RecordingSettings : public SettingsBlock
{
    Q_OBJECT
    using Super = SettingsBlock;
public:

    RecordingSettings(BoolSetting* durationEnabled, TimeSetting* duration);

    void connectWidgets(QCheckBox* durationEnabled, QTimeEdit* duration);

    bool isDurationEnabled() const { return durationEnabled_->getValue(); }

    const QTime& getDuration() const { return duration_->getValue(); }

private:
    BoolSetting* durationEnabled_;
    TimeSetting* duration_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
