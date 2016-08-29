#include "QtGui/QTimeEdit"

#include "TimeSetting.h"

using namespace SideCar::GUI;

TimeSetting::TimeSetting(PresetManager* mgr, const QString& name,
                         bool global)
    : Super(mgr, name, global)
{
    value_ = getOpaqueValue().toTime();
}
    
TimeSetting::TimeSetting(PresetManager* mgr, const QString& name,
                         const QTime& value, bool global)
    : Super(mgr, name, value, global)
{
    value_ = getOpaqueValue().toTime();
}

void
TimeSetting::setValue(const QTime& value)
{
    value_ = value;
    setOpaqueValue(value);
}

void
TimeSetting::valueUpdated()
{
    value_ = getOpaqueValue().toTime();
    emit valueChanged(value_);
}

void
TimeSetting::connectWidget(QTimeEdit* widget)
{
    widget->setTime(value_);
    connect(widget, SIGNAL(timeChanged(const QTime&)),
            SLOT(setValue(const QTime&)));
    connect(this, SIGNAL(valueChanged(const QTime&)),
            widget, SLOT(setTime(const QTime&)));
    Super::connectWidget(widget);
}
