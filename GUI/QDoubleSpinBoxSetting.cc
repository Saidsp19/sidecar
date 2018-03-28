#include "QtGui/QDoubleSpinBox"

#include "QDoubleSpinBoxSetting.h"

using namespace SideCar::GUI;

QDoubleSpinBoxSetting::QDoubleSpinBoxSetting(PresetManager* mgr, QDoubleSpinBox* widget, bool global) :
    DoubleSetting(mgr, widget->objectName(), widget->value(), global), first_(widget)
{
    connectWidget(widget);
}

QDoubleSpinBox*
QDoubleSpinBoxSetting::duplicate(QWidget* parent)
{
    QDoubleSpinBox* widget = new QDoubleSpinBox(parent);
    widget->setRange(first_->minimum(), first_->maximum());
    widget->setValue(first_->value());
    connectWidget(widget);
    return widget;
}
