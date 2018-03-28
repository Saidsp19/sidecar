#include "QTimeEditSetting.h"

using namespace SideCar::GUI;

QTimeEditSetting::QTimeEditSetting(PresetManager* mgr, QTimeEdit* widget, bool global) :
    TimeSetting(mgr, widget->objectName(), widget->time(), global)
{
    connectWidget(widget);
}
