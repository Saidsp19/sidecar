#include "QtWidgets/QCheckBox"

#include "QCheckBoxSetting.h"

using namespace SideCar::GUI;

QCheckBoxSetting::QCheckBoxSetting(PresetManager* mgr, QCheckBox* widget, bool global) :
    BoolSetting(mgr, widget->objectName(), widget->isChecked(), global), first_(widget)
{
    widget->setChecked(getValue());
    connect(widget, SIGNAL(toggled(bool)), this, SLOT(setValue(bool)));
    connect(this, SIGNAL(valueChanged(bool)), widget, SLOT(setChecked(bool)));
}

QCheckBox*
QCheckBoxSetting::duplicate(QWidget* parent)
{
    QCheckBox* widget = new QCheckBox(parent);
    widget->setText(first_->text());
    widget->setChecked(first_->isChecked());
    connect(widget, SIGNAL(toggled(bool)), this, SLOT(setValue(bool)));
    connect(this, SIGNAL(valueChanged(bool)), widget, SLOT(setChecked(bool)));
    return widget;
}
