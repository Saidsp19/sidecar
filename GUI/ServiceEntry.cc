#include "boost/bind.hpp"

#include "Messages/MetaTypeInfo.h"
#include "Zeroconf/Browser.h"

#include "LogUtils.h"
#include "QtMonitor.h"
#include "ServiceEntry.h"

using namespace SideCar;
using namespace SideCar::GUI;

struct ServiceEntry::Private
{
    Zeroconf::ServiceEntry::SignalConnection resolvedConnection_;
};

Logger::Log&
ServiceEntry::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.ServiceEntry");
    return log_;
}

ServiceEntry::ServiceEntry(QObject* parent,
                           const Messages::MetaTypeInfo* metaTypeInfo,
                           const ZCServiceEntryRef& service)
    : QObject(parent), metaTypeInfo_(metaTypeInfo),
      zeroconfServiceEntry_(service),
      name_(QString::fromStdString(service->getName())),
      type_(QString::fromStdString(service->getType())),
      domain_(QString::fromStdString(service->getDomain())),
      fullName_(""), nativeHost_(""), host_(""), transport_(""),
      p_(new Private)
{
    Logger::ProcLog log("ServiceEntry", Log());
    LOGINFO << name_ << ' ' << domain_ << std::endl;
    p_->resolvedConnection_ = 
	service->connectToResolvedSignal(
	    boost::bind(&ServiceEntry::resolvedNotification, this, _1));
}

ServiceEntry::~ServiceEntry()
{
    p_->resolvedConnection_.disconnect();
    delete p_;
}

ServiceEntry*
ServiceEntry::duplicate() const
{
    ServiceEntry* copy = new ServiceEntry(0, metaTypeInfo_,
                                          zeroconfServiceEntry_);
    copy->name_ = name_;
    copy->type_ = type_;
    copy->domain_ = domain_;
    copy->fullName_ = fullName_;
    copy->nativeHost_ = nativeHost_;
    copy->host_ = host_;
    copy->transport_ = transport_;
    return copy;
}

const Zeroconf::ResolvedEntry&
ServiceEntry::getZeroconfResolvedEntry() const
{
    return zeroconfServiceEntry_->getResolvedEntry();
}

uint32_t
ServiceEntry::getInterface() const
{
    return zeroconfServiceEntry_->getInterface();
}

uint16_t
ServiceEntry::getPort() const
{
    return getZeroconfResolvedEntry().getPort();
}

bool
ServiceEntry::isResolved() const
{
    return zeroconfServiceEntry_->isResolved();
}

bool
ServiceEntry::resolve()
{
    if (! isResolved())
	return zeroconfServiceEntry_->resolve();
    emit resolved(this);
    return true;
}

QString
ServiceEntry::getTextEntry(const QString& key) const
{
    return QString::fromStdString(
	getZeroconfResolvedEntry().getTextEntry(key.toStdString()));
}

bool
ServiceEntry::hasTextEntry(const QString& key, QString* value) const
{
    std::string tmp;
    bool rc = getZeroconfResolvedEntry().hasTextEntry(
	key.toStdString(), value ? &tmp : 0);
    if (value) *value = QString::fromStdString(tmp);
    return rc;
}

void
ServiceEntry::resolvedNotification(const ZCServiceEntryRef& service)
{
    static Logger::ProcLog log("resolvedNotification", Log());

    // Obtain the ResolvedEntry object and cache over some of the values as QStrings
    //
    const Zeroconf::ResolvedEntry& resolvedEntry = service->getResolvedEntry();

    fullName_ = QString::fromStdString(resolvedEntry.getFullName());
    nativeHost_ = QString::fromStdString(resolvedEntry.getNativeHost());
    host_ = QString::fromStdString(resolvedEntry.getHost());
    transport_ =
	QString::fromStdString(resolvedEntry.getTextEntry("transport"));

    LOGINFO << "fullName: " << fullName_
	    << " host: " << host_
	    << " port: " << resolvedEntry.getPort()
	    << " transport: " << transport_ << std::endl;

    emit resolved(this);
}
