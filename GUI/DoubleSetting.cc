#include "QtWidgets/QDoubleSpinBox"

#include "DoubleSetting.h"
#include "LogUtils.h"

using namespace SideCar::GUI;

Logger::Log&
DoubleSetting::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.DoubleSetting");
    return log_;
}

DoubleSetting::DoubleSetting(PresetManager* mgr, const QString& name, bool global) : Super(mgr, name, 0.0, global)
{
    static Logger::ProcLog log("DoubleSetting", Log());
    value_ = 0.0;
    LOGINFO << "name: " << name << " value: " << value_ << std::endl;
}

DoubleSetting::DoubleSetting(PresetManager* mgr, const QString& name, double value, bool global) :
    Super(mgr, name, value, global)
{
    static Logger::ProcLog log("DoubleSetting", Log());
    value_ = value;
    LOGINFO << "name: " << name << " value: " << value_ << std::endl;
}

void
DoubleSetting::setValue(double value)
{
    static Logger::ProcLog log("setValue", Log());
    LOGINFO << "name: " << getName() << " old: " << value_ << " new: " << value << std::endl;
    if (value != value_) {
        value_ = value;
        setOpaqueValue(value);
    }
}

void
DoubleSetting::valueUpdated()
{
    static Logger::ProcLog log("valueUpdated", Log());
    value_ = getOpaqueValue().toDouble();
    LOGINFO << "name: " << getName() << " value: " << value_ << std::endl;
    emit valueChanged(value_);
}

void
DoubleSetting::connectWidget(QDoubleSpinBox* widget)
{
    widget->setKeyboardTracking(false);
    widget->setValue(getValue());
    connect(widget, SIGNAL(valueChanged(double)), SLOT(setValue(double)));
    connect(this, SIGNAL(valueChanged(double)), widget, SLOT(setValue(double)));
    Super::connectWidget(widget);
}
