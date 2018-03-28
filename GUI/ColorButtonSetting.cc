#include "ColorButtonSetting.h"
#include "ColorButtonWidget.h"

using namespace SideCar::GUI;

ColorButtonSetting::ColorButtonSetting(PresetManager* mgr, ColorButtonWidget* widget, bool global) :
    Super(mgr, widget->objectName(), widget->getColor(), global)
{
    connectWidget(widget);
}
