#ifndef SIDECAR_GUI_QGROUPBOXSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_QGROUPBOXSETTING_H

#include "QtGui/QGroupBox"

#include "GUI/BoolSetting.h"

namespace SideCar {
namespace GUI {

class QGroupBoxSetting : public BoolSetting {
public:
    QGroupBoxSetting(PresetManager* mgr, QGroupBox* widget, bool global = false);
};

} // end namespace GUI
} // end namespace SideCar

#endif
