#include "ChannelMenu.h"
#include "ChannelSetting.h"
#include "LogUtils.h"
#include "Utils.h"

using namespace SideCar::GUI;

Logger::Log&
ChannelMenu::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.ChannelMenu");
    return log_;
}

ChannelMenu::ChannelMenu(ChannelSetting* setting, QWidget* parent) : Super(parent), setting_(setting), names_()
{
    connect(this, SIGNAL(triggered(QAction*)), SLOT(itemSelected(QAction*)));
    connect(setting, SIGNAL(availableChannels(const QList<QString>&)), SLOT(setChannelNames(const QList<QString>&)));
    connect(setting, SIGNAL(valueChanged(const QString&)), SLOT(setActive(const QString&)));
    addAction(" ");
}

void
ChannelMenu::setChannelNames(const QList<QString>& names)
{
    Logger::ProcLog log("setChannelNames", Log());
    LOGINFO << names.size() << std::endl;

    clear();

    QString active = setting_->getValue();
    for (int index = 0; index < names.size(); ++index) {
        const QString& name(names[index]);
        LOGDEBUG << "adding " << name << std::endl;
        QAction* action = addAction(names[index]);
        action->setCheckable(true);
        action->setChecked(active == name);
    }

    names_ = names;
}

void
ChannelMenu::itemSelected(QAction* action)
{
    Logger::ProcLog log("itemSelected", Log());
    LOGINFO << "action: " << action->text() << std::endl;
    if (!names_.empty()) {
        if (action->text() != setting_->getValue()) { setting_->setValue(action->text()); }
    }
}

void
ChannelMenu::setActive(const QString& channelName)
{
    Logger::ProcLog log("setActive", Log());
    LOGINFO << "channelName: " << channelName << std::endl;
    QList<QAction*> acts = actions();
    for (int index = 0; index < acts.size(); ++index) { acts[index]->setChecked(acts[index]->text() == channelName); }
}
