#ifndef SIDECAR_GUI_MASTER_DURATIONSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_DURATIONSETTING_H

#include "GUI/QTimeEditSetting.h"

namespace SideCar {
namespace GUI {
namespace Master {

class DurationSetting : public QTimeEditSetting {
    Q_OBJECT
    using Super = QTimeEditSetting;

public:
    DurationSetting(PresetManager* mgr, QTimeEdit* widget, bool global = false);

    int getDuration() const { return duration_; }

private:
    void valueUpdated();

    int duration_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
