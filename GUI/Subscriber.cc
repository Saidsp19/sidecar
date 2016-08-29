#include <sys/types.h>
#include <unistd.h>

#include "IO/ZeroconfRegistry.h"

#include "GUI/QtMonitor.h"
#include "Messages/MetaTypeInfo.h"

#include "LogUtils.h"
#include "ReaderThread.h"
#include "ServiceEntry.h"
#include "Subscriber.h"

using namespace SideCar::GUI;

Logger::Log&
Subscriber::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.Subscriber");
    return log_;
}

Subscriber::Subscriber(QObject* parent)
    : QObject(parent), serviceEntry_(0), reader_(new ReaderThread),
      connected_(false)
{
    static Logger::ProcLog log("Subscriber", Log());
    LOGINFO << std::endl;
    connect(reader_, SIGNAL(connected()), SLOT(readerConnected()));
    connect(reader_, SIGNAL(dataAvailable()), SIGNAL(dataAvailable()));
    connect(reader_, SIGNAL(disconnected()), SLOT(readerDisconnected()));
}

Subscriber::~Subscriber()
{
    static Logger::ProcLog log("~Subscriber", Log());
    LOGINFO << std::endl;
    reader_->stop();
    delete reader_;
}

void
Subscriber::shutdown()
{
    static Logger::ProcLog log("shutdown", Log());
    LOGINFO << std::endl;
    useServiceEntry(0);
}

void
Subscriber::useServiceEntry(ServiceEntry* serviceEntry)
{
    static Logger::ProcLog log("useServiceEntry", Log());
    LOGINFO << "new: " << serviceEntry << " old: " << serviceEntry_
	    << std::endl;

    if (serviceEntry_ == serviceEntry)
	return;

    if (serviceEntry_) {
	LOGDEBUG << "disconnecting old service entry" << std::endl;
	serviceEntry_->disconnect(this);
	serviceEntry_ = 0;
	reader_->stop();
	if (connected_) {
	    connected_ = false;
	    emit disconnected();
	}
    }

    if (serviceEntry) {
	LOGDEBUG << "new service entry: " << serviceEntry << ' '
		 << serviceEntry->getName() << std::endl;
	serviceEntry_ = serviceEntry;
	connect(serviceEntry, SIGNAL(resolved(ServiceEntry*)), this,
                SLOT(resolvedServiceEntry(ServiceEntry*)));
	serviceEntry->resolve();
    }
}

void
Subscriber::resolvedServiceEntry(ServiceEntry* serviceEntry)
{
    static Logger::ProcLog log("resolvedServiceEntry", Log());
    LOGINFO << serviceEntry << ' ' << serviceEntry->getName() << std::endl;
    connect(serviceEntry, SIGNAL(destroyed()), this,
            SLOT(lostServiceEntry()));
    reader_->useServiceEntry(serviceEntry_);
}

void
Subscriber::lostServiceEntry()
{
    static Logger::ProcLog log("lostServiceEntry", Log());
    LOGINFO << sender() << ' ' << serviceEntry_ << std::endl;
    if (serviceEntry_ != sender()) {
	LOGFATAL << "invalid notification" << std::endl;
	return;
    }

    useServiceEntry(0);
}

void
Subscriber::readerConnected()
{
    Logger::ProcLog log("readerConnected", Log());
    LOGINFO << serviceEntry_->getName() << std::endl;
    if (! connected_) {
	connected_ = true;
	emit connected();
    }
}

void
Subscriber::readerDisconnected()
{
    Logger::ProcLog log("readerDisconnected", Log());
    LOGINFO << std::endl;
    if (connected_) {
	connected_ = false;
	emit disconnected();
    }
}

const MessageList&
Subscriber::getMessages() const
{
    return reader_->getMessages();
}
