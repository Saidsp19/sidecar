#include "ColorButtonWidget.h"
#include "LogUtils.h"
#include "QColorSetting.h"

using namespace SideCar::GUI;

QColorSetting::QColorSetting(PresetManager* mgr, const QString& name,
                             const QColor& value, bool global)
    : Super(mgr, name, value, global), value_(value)
{
    ;
}

void
QColorSetting::setValue(const QColor& value)
{
    if (value != value_) {
	value_ = value;
	setOpaqueValue(value);
    }
}

void
QColorSetting::valueUpdated()
{
    value_ = getOpaqueValue().value<QColor>();
    emit valueChanged(value_);
}

void
QColorSetting::connectWidget(ColorButtonWidget* widget)
{
    widget->setColor(getValue());
    connect(widget, SIGNAL(colorChanged(const QColor&)),
            SLOT(setValue(const QColor&)));
    connect(this, SIGNAL(valueChanged(const QColor&)), widget,
            SLOT(setColor(const QColor&)));
}
