#include "QtWidgets/QGroupBox"

#include "QGroupBoxSetting.h"

using namespace SideCar::GUI;

QGroupBoxSetting::QGroupBoxSetting(PresetManager* mgr, QGroupBox* widget, bool global) :
    BoolSetting(mgr, widget->objectName(), widget->isChecked(), global)
{
    widget->setChecked(getValue());
    connect(widget, SIGNAL(toggled(bool)), this, SLOT(setValue(bool)));
    connect(this, SIGNAL(valueChanged(bool)), widget, SLOT(setChecked(bool)));
}
