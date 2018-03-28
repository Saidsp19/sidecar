#include "QtGui/QSpinBox"

#include "QSpinBoxSetting.h"

using namespace SideCar::GUI;

QSpinBoxSetting::QSpinBoxSetting(PresetManager* mgr, QSpinBox* widget, bool global) :
    IntSetting(mgr, widget->objectName(), widget->value(), global), first_(widget)
{
    connectWidget(widget);
}

QSpinBox*
QSpinBoxSetting::duplicate(QWidget* parent)
{
    QSpinBox* widget = new QSpinBox(parent);
    widget->setRange(first_->minimum(), first_->maximum());
    widget->setValue(first_->value());
    connectWidget(widget);
    return widget;
}
