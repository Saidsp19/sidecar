#include "StringSetting.h"

using namespace SideCar::GUI;

StringSetting::StringSetting(PresetManager* mgr, const QString& name, bool global) : Super(mgr, name, "", global)
{
    value_ = "";
}

StringSetting::StringSetting(PresetManager* mgr, const QString& name, const QString& value, bool global) :
    Super(mgr, name, value, global)
{
    value_ = value;
}

void
StringSetting::setValue(const QString& value)
{
    value_ = value;
    setOpaqueValue(value);
}

void
StringSetting::valueUpdated()
{
    value_ = getOpaqueValue().toString();
    emit valueChanged(value_);
}
