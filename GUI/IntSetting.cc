#include "QtWidgets/QComboBox"
#include "QtWidgets/QSpinBox"

#include "IntSetting.h"

using namespace SideCar::GUI;

IntSetting::IntSetting(PresetManager* mgr, const QString& name, bool global)
    : Super(mgr, name, 0, global), value_(0)
{
    ;
}

IntSetting::IntSetting(PresetManager* mgr, const QString& name, int value, bool global) :
    Super(mgr, name, value, global), value_(value)
{
    ;
}

void
IntSetting::setValue(int value)
{
    if (value != value_) {
        value_ = value;
        setOpaqueValue(value);
    }
}

void
IntSetting::valueUpdated()
{
    value_ = getOpaqueValue().toInt();
    emit valueChanged(value_);
}

void
IntSetting::connectWidget(QSpinBox* widget)
{
    widget->setKeyboardTracking(false);
    widget->setValue(getValue());
    connect(widget, SIGNAL(valueChanged(int)), SLOT(setValue(int)));
    connect(this, SIGNAL(valueChanged(int)), widget, SLOT(setValue(int)));
    Super::connectWidget(widget);
}

void
IntSetting::connectWidget(QComboBox* widget)
{
    widget->setCurrentIndex(getValue());
    connect(widget, SIGNAL(activated(int)), SLOT(setValue(int)));
    connect(this, SIGNAL(valueChanged(int)), widget, SLOT(setCurrentIndex(int)));
    Super::connectWidget(widget);
}
