#include "QtCore/QList"
#include "QtCore/QSettings"
#include "QtGui/QComboBox"

#include "IO/ZeroconfRegistry.h"

#include "ChannelGroup.h"
#include "LogUtils.h"
#include "MessageList.h"
#include "ServiceBrowser.h"
#include "ServiceEntry.h"
#include "Subscriber.h"

using namespace SideCar::GUI;

Logger::Log&
ChannelGroup::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.ChannelGroup");
    return log_;
}

ChannelGroup::ChannelGroup(QObject* parent, const std::string& typeName, QComboBox* found)
    : QObject(parent), typeName_(typeName), 
      browser_(new ServiceBrowser(this, QString::fromStdString(MakeTwinZeroconfType(typeName_)))),
      activeServiceEntry_(0), found_(found), subscriber_(new Subscriber(this)),
      settingsKey_(QString::fromStdString(typeName_))
{
    static Logger::ProcLog log("ChannelGroup", Log());
    LOGINFO << typeName_ << std::endl;

    settingsKey_ += "LastConnection";

    found_->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    connect(browser_, SIGNAL(availableServices(const ServiceEntryHash&)),
            SLOT(setAvailableServices(const ServiceEntryHash&)));
    connect(found_, SIGNAL(activated(int)), SLOT(selectionChanged(int)));
    connect(subscriber_, SIGNAL(dataAvailable()), SLOT(dataAvailable()));

    browser_->start();
}

void
ChannelGroup::shutdown()
{
    static Logger::ProcLog log("shutdown", Log());
    LOGINFO << typeName_ << std::endl;
    subscriber_->shutdown();
    browser_->stop();
}

void
ChannelGroup::setAvailableServices(const ServiceEntryHash& services)
{
    static Logger::ProcLog log("setAvailableServices", Log());
    LOGINFO << typeName_ << ' ' << services.size() << std::endl;

    QString current;

    // If nothing is selected in the widget, fetch the name of the last channel that was active, and search for
    // it as we load the new service names.
    //
    if (found_->currentIndex() < 1) {
	QSettings settings;
	current = settings.value(settingsKey_).toString();
	LOGDEBUG << "previous current: " << current << std::endl;
    }
    else {
	current = found_->currentText();
    }

    LOGDEBUG << "current: " << current << std::endl;

    found_->clear();

    // Get the found keys and sort then alphabetically.
    //
    int active = 0;
    QList<QString> keys = services.keys();
    qSort(keys.begin(), keys.end());

    // Add to the widget, remembering which one matches the one we would like to be current.
    //
    found_->addItem(" ");
    for (int index = 0; index < keys.size(); ++index) {
	if (keys[index] == current) active = index + 1;
	found_->addItem(keys[index]);
    }

    found_->setEnabled(! keys.empty());
    found_->setCurrentIndex(active);
    changeChannels(active);
}

void
ChannelGroup::selectionChanged(int index)
{
    static Logger::ProcLog log("selectionChanged", Log());
    LOGINFO << typeName_ << ' ' << index << std::endl;

    // Act on the channel change request, and remember the name of the new active channel.
    //
    changeChannels(index);
    QSettings settings;
    settings.setValue(settingsKey_, found_->currentText());
}

void
ChannelGroup::changeChannels(int index)
{
    ServiceEntry* serviceEntry = 0;
    if (index) {
	serviceEntry = browser_->getServiceEntry(found_->currentText());
    }

    if (activeServiceEntry_ != serviceEntry) {
	subscriber_->useServiceEntry(serviceEntry);
	activeServiceEntry_ = serviceEntry;
	emit channelChanged();
    }
}

void
ChannelGroup::dataAvailable()
{
    emit incoming(subscriber_->getMessages());
}
