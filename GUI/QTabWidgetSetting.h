#ifndef SIDECAR_GUI_QTABWIDGETSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_QTABWIDGETSETTING_H

#include "QtGui/QTabWidget"

#include "GUI/IntSetting.h"

namespace SideCar {
namespace GUI {

class QTabWidgetSetting : public IntSetting
{
public:
    QTabWidgetSetting(PresetManager* mgr, QTabWidget* widget,
                      bool global = false);
};

} // end namespace GUI
} // end namespace SideCar

#endif
