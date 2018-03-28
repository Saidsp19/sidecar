#include "QtCore/QTimer"

#include "Setting.h"
#include "SettingsBlock.h"

using namespace SideCar::GUI;

void
SettingsBlock::postSettingChanged()
{
    if (!pendingNotification_) {
        pendingNotification_ = true;
        QTimer::singleShot(0, this, SLOT(emitSettingChanged()));
    }
}

void
SettingsBlock::add(Setting* setting)
{
    connect(setting, SIGNAL(valueChanged()), this, SLOT(postSettingChanged()));
}

void
SettingsBlock::emitSettingChanged()
{
    pendingNotification_ = false;
    emit settingChanged();
}
