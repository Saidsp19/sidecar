#include "GUI/ServiceEntry.h"
#include "GUI/Subscriber.h"
#include "GUI/LogUtils.h"

#include "App.h"
#include "ChannelConnection.h"
#include "ChannelPlotSettings.h"

using namespace SideCar::GUI::HealthAndStatus;

Logger::Log&
ChannelConnection::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("hands.ChannelConnection");
    return log_;
}

ChannelConnection::ChannelConnection(const QString& name, QObject* parent)
    : Super(parent), 
      settings_(new ChannelPlotSettings(App::GetApp()->getPresetManager(),
                                        this, name)),
      reader_(0), name_(name)
{
    setObjectName(QString("ChannelConnection ") + name);
}

ChannelConnection::~ChannelConnection()
{
    shutdown();
}

bool
ChannelConnection::isConnected() const
{
    return reader_ && reader_->isConnected();
}

void
ChannelConnection::shutdown()
{
    Logger::ProcLog log("shutdown", Log());
    LOGERROR << std::endl;
    if (reader_) {
	reader_->shutdown();
	reader_->deleteLater();
	reader_ = 0;
    }
}

void
ChannelConnection::readerIncoming()
{
    if (reader_) {
	const MessageList& messages(reader_->getMessages());
	emit incoming(messages);
    }
}

void
ChannelConnection::useServiceEntry(ServiceEntry* serviceEntry)
{
    if (! reader_) {
	reader_ = new Subscriber(this);
	connect(reader_, SIGNAL(connected()), SIGNAL(connected()));
	connect(reader_, SIGNAL(disconnected()), SIGNAL(disconnected()));
	connect(reader_, SIGNAL(dataAvailable()), SLOT(readerIncoming()));
    }
    reader_->useServiceEntry(serviceEntry);
}
