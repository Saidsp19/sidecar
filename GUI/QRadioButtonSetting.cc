#include "QRadioButtonSetting.h"

using namespace SideCar::GUI;

QRadioButtonSetting::QRadioButtonSetting(PresetManager* mgr, QRadioButton* widget, bool global) :
    BoolSetting(mgr, widget->objectName(), widget->isChecked(), global)
{
    connectWidget(widget);
}
