#include "StringListSetting.h"

using namespace SideCar::GUI;

StringListSetting::StringListSetting(PresetManager* mgr, const QString& name,
                                     bool global)
    : Super(mgr, name, QStringList(), global)
{
    value_ = getOpaqueValue().toStringList();
}

StringListSetting::StringListSetting(PresetManager* mgr, const QString& name,
                                     const QStringList& value,
                                     bool global)
    : Super(mgr, name, value, global)
{
    value_ = getOpaqueValue().toStringList();
}

void
StringListSetting::setValue(const QStringList& value)
{
    value_ = value;
    setOpaqueValue(value);
}

void
StringListSetting::valueUpdated()
{
    value_ = getOpaqueValue().toStringList();
    emit valueChanged(value_);
}
