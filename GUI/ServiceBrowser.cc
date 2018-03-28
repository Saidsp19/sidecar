#include "Utils/Utils.h"
#include "Zeroconf/Browser.h"

#include "LogUtils.h"
#include "QtMonitor.h"
#include "ServiceBrowser.h"
#include "ServiceEntry.h"

using namespace SideCar;
using namespace SideCar::GUI;

QtMonitorFactory::Ref ServiceBrowser::monitorFactory_ = QtMonitorFactory::Make();

Logger::Log&
ServiceBrowser::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.ServiceBrowser");
    return log_;
}

ServiceBrowser::ServiceBrowser(QObject* parent, const QString& type) :
    QObject(parent), browser_(Zeroconf::Browser::Make(monitorFactory_, type.toStdString())), active_(), type_(type),
    metaTypeInfo_(0)
{
    Logger::ProcLog log("ServiceBrowser", Log());
    browser_->connectToFoundSignal([this](auto& s) { foundNotification(s); });
    browser_->connectToLostSignal([this](auto& s) { lostNotification(s); });
    auto index = type.indexOf(',');
    if (index != -1) {
        QString subType(type.mid(index + 2));
        metaTypeInfo_ = Messages::MetaTypeInfo::Find(subType.toStdString());
    }
}

void
ServiceBrowser::start()
{
    Logger::ProcLog log("start", Log());
    LOGINFO << std::endl;
    browser_->start();
}

void
ServiceBrowser::stop()
{
    Logger::ProcLog log("stop", Log());
    LOGINFO << std::endl;
    browser_->stop();
}

void
ServiceBrowser::foundNotification(const ZCServiceEntryVector& services)
{
    Logger::ProcLog log("foundNotification", Log());
    LOGINFO << services.size() << std::endl;

    ServiceEntryList found;
    for (auto service : services) {
        auto name = QString::fromStdString(service->getName());
        LOGDEBUG << "name: " << name << std::endl;

        auto pos = active_.find(name);
        if (pos != active_.end()) {
            LOGDEBUG << "duplicate service entry with name " << name << " - interface: " << service->getInterface()
                     << std::endl;
            continue;
        }

        auto entry = new ServiceEntry(this, metaTypeInfo_, service);
        active_[name] = entry;
        found.append(entry);
    }

    if (!found.empty()) {
        emit foundServices(found);
        emit availableServices(active_);
    }
}

void
ServiceBrowser::lostNotification(const ZCServiceEntryVector& services)
{
    Logger::ProcLog log("lostNotification", Log());
    LOGINFO << services.size() << std::endl;

    ServiceEntryList lost;
    for (auto service : services) {
        auto name = QString::fromStdString(service->getName());
        LOGDEBUG << "name: " << name << std::endl;

        auto pos = active_.find(name);
        if (pos == active_.end()) {
            LOGDEBUG << "service entry with name " << name << " not found"
                     << " - interface: " << service->getInterface() << std::endl;
            continue;
        }

        if (pos.value()->getInterface() == service->getInterface()) {
            lost.append(pos.value());
            active_.erase(pos);
        }
    }

    if (!lost.empty()) {
        emit lostServices(lost);
        emit availableServices(active_);
        std::for_each(lost.begin(), lost.end(), [](auto v) { delete v; });
    }
}

ServiceEntry*
ServiceBrowser::getServiceEntry(const QString& name) const
{
    Logger::ProcLog log("getServiceEntry", Log());
    ServiceEntryHash::const_iterator pos = active_.find(name);
    ServiceEntry* entry = pos != active_.end() ? pos.value() : 0;
    LOGDEBUG << "name: " << name << " found: " << entry << std::endl;
    return entry;
}
