#include "QtWidgets/QTabWidget"

#include "QTabWidgetSetting.h"

using namespace SideCar::GUI;

QTabWidgetSetting::QTabWidgetSetting(PresetManager* mgr, QTabWidget* widget, bool global) :
    IntSetting(mgr, widget->objectName(), widget->currentIndex(), global)
{
    widget->setCurrentIndex(getValue());
    connect(widget, SIGNAL(currentChanged(int)), this, SLOT(setValue(int)));
    connect(this, SIGNAL(valueChanged(int)), widget, SLOT(setCurrentIndex(int)));
}
