#include "QtCore/QTimer"
#include "QtGui/QWidget"

#include "LogUtils.h"
#include "PresetManager.h"
#include "Setting.h"

using namespace SideCar::GUI;

Logger::Log&
Setting::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.Setting");
    return log_;
}

Setting::Setting(PresetManager* mgr, const QString& name, bool global) :
    QObject(mgr), name_(name), value_(), index_(-1), enabled_(true)
{
    static Logger::ProcLog log("Setting", Log());
    LOGINFO << "name: " << name << " global: " << global << std::endl;
    index_ = mgr->addSetting(this, global);
}

Setting::Setting(PresetManager* mgr, const QString& name, const QVariant& value, bool global) :
    QObject(mgr), name_(name), value_(value), index_(-1), enabled_(true)
{
    static Logger::ProcLog log("Setting", Log());
    LOGINFO << "name: " << name << " global: " << global << std::endl;
    // QTimer::singleShot(0, this, SLOT(registerSetting()));
    index_ = mgr->addSetting(this, global);
}

void
Setting::restoreValue(const QVariant& value)
{
    static Logger::ProcLog log("restoreValue", Log());
    if (value_ != value) {
        LOGINFO << name_ << " restoring" << std::endl;
        value_ = value;
        valueUpdated();
        emit valueChanged();
    }
}

void
Setting::setOpaqueValue(const QVariant& value)
{
    static Logger::ProcLog log("setQpaqueValue", Log());
    if (value_ != value) {
        LOGINFO << name_ << " updating" << std::endl;
        value_ = value;
        valueUpdated();
        emit valueChanged();
        if (index_ != -1) emit save(index_, value_);
    }
}

void
Setting::setEnabled(bool state)
{
    if (state != enabled_) {
        enabled_ = state;
        emit enabledChanged(state);
    }
}

void
Setting::connectWidget(QWidget* widget)
{
    connect(this, SIGNAL(enabledChanged(bool)), widget, SLOT(setEnabled(bool)));
    widget->setEnabled(enabled_);
}

void
Setting::disconnectWidget(QWidget* widget)
{
    widget->disconnect(this);
    disconnect(widget);
}
