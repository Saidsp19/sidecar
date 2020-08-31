#include <algorithm>

#include "QtCore/QList"
#include "QtCore/QString"
#include "QtWidgets/QApplication"
#include "QtWidgets/QComboBox"

#include "AppBase.h"
#include "ChannelSetting.h"
#include "LogUtils.h"
#include "MessageList.h"
#include "ServiceEntry.h"
#include "Subscriber.h"
#include "Utils.h"

using namespace SideCar::GUI;

Logger::Log&
ChannelSetting::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.ChannelSetting");
    return log_;
}

QString
ChannelSetting::GetChannelName(const std::string& typeName)
{
    return QString("%1 Channel").arg(QString::fromStdString(typeName));
}

ChannelSetting::ChannelSetting(const std::string& typeName, PresetManager* mgr, QComboBox* widget, bool global) :
    Super(mgr, GetChannelName(typeName), "-OFF-", global),
    browser_(new ServiceBrowser(this, QString::fromStdString(MakeTwinZeroconfType(typeName)))), activeServiceEntry_(0),
    first_(0), subscriber_(new Subscriber(this))
{
    static Logger::ProcLog log("ChannelSetting", Log());
    LOGINFO << getName() << std::endl;

    if (widget) connectWidget(widget);

    connect(browser_, SIGNAL(availableServices(const ServiceEntryHash&)), this,
            SLOT(setAvailableServices(const ServiceEntryHash&)));
    connect(subscriber_, SIGNAL(dataAvailable()), this, SLOT(dataAvailable()));

    if (AppBase::GetApp()) connect(AppBase::GetApp(), SIGNAL(shutdown()), SLOT(shutdown()));

    browser_->start();
}

void
ChannelSetting::connectWidget(QComboBox* widget)
{
    if (!first_) {
        first_ = widget;
    } else {
        widget->setEnabled(first_->isEnabled());
        widget->setModel(first_->model());
        widget->setCurrentIndex(first_->currentIndex());
    }

    widget->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(widget, SIGNAL(activated(const QString&)), this, SLOT(setValue(const QString&)));
    connect(this, SIGNAL(valueChanged(int)), widget, SLOT(setCurrentIndex(int)));
    connect(this, SIGNAL(channelsAvailable(bool)), widget, SLOT(setEnabled(bool)));

    Super::connectWidget(widget);
}

void
ChannelSetting::valueUpdated()
{
    static Logger::ProcLog log("valueUpdated", Log());
    Super::valueUpdated();

    // Get the new setting value.
    //
    QString value = getValue();
    LOGINFO << getName() << " value: " << value << std::endl;
    if (!first_) return;

    // Locate the setting value in the list of available channels. If not found, use the very first entry, which
    // is a blank line.
    //
    int index = first_->findText(value);
    LOGDEBUG << "index: " << index << std::endl;
    if (index == -1) index = 0;

    // Locate the Zeroconf service entry that corresponds to the channel name.
    //
    ServiceEntry* serviceEntry = index ? browser_->getServiceEntry(value) : 0;

    // Hand the new ServiceEntry to our subscriber thread.
    //
    if (activeServiceEntry_ != serviceEntry) {
        subscriber_->useServiceEntry(serviceEntry);
        activeServiceEntry_ = serviceEntry;
    }

    // Notify widgets to show the new entry
    //
    emit valueChanged(index);
}

void
ChannelSetting::shutdown()
{
    static Logger::ProcLog log("shutdown", Log());
    LOGINFO << getName() << std::endl;

    // Application is shutting down. Disconnect subscriber and stop browsing.
    //
    subscriber_->shutdown();
    browser_->stop();
}

void
ChannelSetting::setAvailableServices(const ServiceEntryHash& services)
{
    static Logger::ProcLog log("setAvailableServices", Log());
    LOGINFO << getName() << ' ' << services.size() << std::endl;

    // If nothing is selected in the widget, fetch the name of the last channel that was active, and search for
    // it as we load the new service names. Otherwise, use current selection.
    //
    QString wanted;
    if (!first_ || first_->currentIndex() < 1) {
        wanted = getValue();
    } else {
        wanted = first_->currentText();
    }

    LOGDEBUG << "wanted: " << wanted << std::endl;

    // Get the found keys and sort then alphabetically.
    //
    QList<QString> keys = services.keys();
    std::sort(keys.begin(), keys.end());

    // Place an empty name at the beginning of the list to represent no channel selection or OFF.
    //
    keys.prepend("-OFF-");

    // Notify others of the new key set
    //
    emit availableChannels(keys);

    // Add to the widget, remembering which one matches the one we would like to be current.
    //
    if (first_) {
        int found = 0;
        first_->clear();
        for (int index = 0; index < keys.size(); ++index) {
            LOGDEBUG << "key: " << keys[index] << std::endl;
            if (keys[index] == wanted) { found = index; }
            first_->addItem(keys[index]);
        }

        LOGDEBUG << "found: " << found << " wanted: " << wanted << std::endl;

        // Locate the Zeroconf service entry that corresponds to the channel name.
        //
        ServiceEntry* serviceEntry = found ? browser_->getServiceEntry(wanted) : 0;

        // Update our Subscriber if the ServiceEntry we wanted was lost or appeared.
        //
        if (activeServiceEntry_ != serviceEntry) {
            subscriber_->useServiceEntry(serviceEntry);
            activeServiceEntry_ = serviceEntry;
            if (serviceEntry) Super::valueUpdated();
        }

        // Notify attached QComboBox widgets of the new index to show.
        //
        emit valueChanged(found);
    }

    emit channelsAvailable(keys.size() > 1);
}

void
ChannelSetting::dataAvailable()
{
    emit incoming(subscriber_->getMessages());
}

bool
ChannelSetting::hasChannels() const
{
    return first_ && first_->count() > 1;
}

bool
ChannelSetting::isConnected() const
{
    return first_ && first_->currentIndex() > 0;
}
