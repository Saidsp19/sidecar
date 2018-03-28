#include "QtCore/QSettings"
#include "QtCore/QStringList"

#include "LogUtils.h"
#include "Preset.h"
#include "PresetManager.h"
#include "Setting.h"

using namespace SideCar::GUI;

Logger::Log&
Preset::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.Preset");
    return log_;
}

Preset::Preset(const QString& name, PresetManager* parent) : QObject(parent), name_(name), values_(), dirty_(false)
{
    Logger::ProcLog log("Preset", Log());
    LOGINFO << name << std::endl;
}

Preset*
Preset::duplicate(const QString& name)
{
    Preset* preset = new Preset(name, getParent());
    preset->values_ = values_;
    return preset;
}

void
Preset::setValue(int index, const QVariant& value)
{
    static Logger::ProcLog log("setValue", Log());
    LOGINFO << name_ << std::endl;

    while (values_.size() <= index) values_.append(QVariant());

    if (value == values_[index]) {
        ;
    } else if (!values_[index].isValid()) {
        values_[index] = value;
    } else {
        if (!getParent()->isRestoring() && !dirty_) {
            LOGDEBUG << "*** Preset: " << name_ << " setting: " << getParent()->getSetting(index)->getName()
                     << " old: '" << values_[index].toString() << "' (" << values_[index].typeName() << ") new: '"
                     << value.toString() << "' (" << value.typeName() << ")" << std::endl;
            dirty_ = true;
        }
        values_[index] = value;
    }
}

void
Preset::save(QSettings& file, const QStringList& names)
{
    static Logger::ProcLog log("save", Log());
    LOGINFO << "name: " << name_ << std::endl;

    file.beginGroup(name_);
    {
        for (int index = 0; index < names.size(); ++index) {
            LOGINFO << "name: " << names[index] << " value: " << values_[index].toString() << std::endl;
            file.setValue(names[index], values_[index]);
        }
    }
    file.endGroup();

    dirty_ = false;
}

void
Preset::restore(QSettings& file, const QList<Setting*>& settings, bool apply)
{
    static Logger::ProcLog log("restore", Log());
    LOGINFO << "restoring " << name_ << " apply: " << apply << std::endl;

    values_.clear();

    bool isOldVersion = getParent()->isOldVersion();

    file.beginGroup(name_);
    {
        for (int index = 0; index < settings.size(); ++index) {
            const QString& name = settings[index]->getName();
            QVariant value = file.value(name);

            bool doApply = apply;
            if (!value.isValid()) {
                doApply = false;
                value = settings[index]->getOpaqueValue();
            } else if (isOldVersion) {
                value = getParent()->upgradeSetting(name, value);
            }

            LOGDEBUG << "name: " << name << " value: " << value.toString() << " doApply: " << doApply << std::endl;

            values_.append(value);

            if (doApply) settings[index]->restoreValue(value);
        }
    }
    file.endGroup();

    dirty_ = false;
}

void
Preset::restoreOne(QSettings& file, Setting* setting)
{
    static Logger::ProcLog log("restoreOne", Log());
    LOGINFO << "restoring " << name_ << std::endl;

    file.beginGroup(name_);
    {
        QVariant value = file.value(setting->getName());
        bool ok = value.isValid();
        if (!ok) value = setting->getOpaqueValue();

        LOGDEBUG << "name: " << setting->getName() << " value: " << value.toString() << std::endl;

        values_.append(value);

        if (ok) setting->restoreValue(value);
    }
    file.endGroup();
}

void
Preset::apply(const QList<Setting*>& settings)
{
    static Logger::ProcLog log("apply", Log());
    LOGINFO << "name: " << name_ << std::endl;

    for (int index = 0; index < settings.size(); ++index) { settings[index]->restoreValue(values_[index]); }
}

PresetManager*
Preset::getParent() const
{
    return qobject_cast<PresetManager*>(parent());
}

void
Preset::expand(int index, const QVariant& value)
{
    while (values_.size() < index) values_.append(QVariant());
    values_.append(value);
}
