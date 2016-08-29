#include "Zeroconf/Publisher.h"	// NOTE: include before any Qt includes

#include "QtNetwork/QHostAddress"
#include "QtNetwork/QHostInfo"

#include "IO/MessageManager.h"
#include "Utils/Format.h"	// for Utils::showErrno

#include "LogUtils.h"
#include "MulticastMessageReader.h"
#include "QtMonitor.h"
#include "ServiceEntry.h"
#include "Utils.h"

using namespace SideCar::IO;
using namespace SideCar::GUI;

Logger::Log&
MulticastMessageReader::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.MulticastMessageReader");
    return log_;
}

MulticastMessageReader*
MulticastMessageReader::Make(const ServiceEntry* service)
{
    static Logger::ProcLog log("Make", Log());
    LOGINFO << std::endl;

    MulticastMessageReader* reader =
	new MulticastMessageReader(service->getMetaTypeInfo());
    if (! reader->open(service)) {
	LOGERROR << "failed to open reader" << std::endl;
	delete reader;
	return 0;
    }

    return reader;
}

MulticastMessageReader::MulticastMessageReader(
    const Messages::MetaTypeInfo* metaTypeInfo)
    : Super(metaTypeInfo), proxy_(0), heartBeatWriter_(0), timer_(0),
      reader_(), connected_(false)
{
    static Logger::ProcLog log("MulticastMessageReader", Log());
    LOGINFO << std::endl;
}

MulticastMessageReader::~MulticastMessageReader()
{
    static Logger::ProcLog log("~MulticastMessageReader", Log());
    LOGINFO << std::endl;
    if (timer_)
	timer_->stop();
    if (heartBeatWriter_)
	sendHeartBeat("BYE");
    reader_.getDevice().close();
    delete proxy_;
    proxy_ = 0;
}

bool
MulticastMessageReader::open(const ServiceEntry* service)
{
    static Logger::ProcLog log("open", Log());

    // Attempt to join the multicast group defined by the host/port of the service.
    //
    port_ = service->getPort();
    LOGINFO << "host: " << service->getHost() << " port: " << port_
	    << std::endl;
    ACE_INET_Addr addr;
    addr.set(port_, service->getHost().toStdString().c_str(), 1, PF_INET);
    if (! reader_.join(addr)) {
	LOGERROR << "failed to open multicast socket - " << Utils::showErrno()
		 << std::endl;
	return false;
    }

    // Create a proxy object that we can connect slots to.
    //
    Q_ASSERT(proxy_ == 0);
    proxy_ = new QUdpSocket(this);
    LOGDEBUG << "reader socket: " << reader_.getDevice().get_handle()
	     << std::endl;
    if (! proxy_->setSocketDescriptor(reader_.getDevice().get_handle())) {
	LOGERROR << "failed setSocketDescriptor - "
		 << proxy_->error() << ' ' << proxy_->errorString()
		 << std::endl;
	return false;
    }

    connect(proxy_, SIGNAL(readyRead()), SLOT(socketReadyRead()));
    connect(proxy_, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(socketError(QAbstractSocket::SocketError)));

    heartBeatWriter_ = new QUdpSocket(this);
    if (! heartBeatWriter_->bind()) {
	LOGERROR << "failed heartBeatWriter_->bind()" << std::endl;
    }

    heartBeatHost_.clear();
    QHostInfo hostInfo = QHostInfo::fromName(service->getNativeHost());
    if (hostInfo.error() != QHostInfo::NoError) {
	LOGERROR << "failed QHostInfo::fromName() with "
		 << service->getNativeHost() << std::endl;
	heartBeatHost_ = "127.0.0.1";
    }
    else {
	foreach (QHostAddress address, hostInfo.addresses()) {
	    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
		heartBeatHost_ = address;
		break;
	    }
	}
    }

    QString portText = service->getTextEntry("HeartBeatPort");
    heartBeatPort_ = portText.toUShort();
    LOGDEBUG << "heartBeatHost: " << heartBeatHost_.toString()
	     << " port: " << heartBeatPort_ << std::endl;

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), SLOT(beatHeart()));
    timer_->start(5000);

    QTimer::singleShot(0, this, SLOT(beatHeart()));

    timer_ = new QTimer(this);

    return true;
}

void
MulticastMessageReader::sendHeartBeat(const char* msg)
{
    static Logger::ProcLog log("sendHeartBeat", Log());
    LOGINFO << "msg: " << msg << std::endl;
    qint64 count = ::strlen(msg) + 1;
    if (heartBeatWriter_) {
	if (heartBeatWriter_->writeDatagram(msg, count, heartBeatHost_,
                                            heartBeatPort_) != count) {
	    LOGERROR << "failed to write heart-beat message to "
		     << heartBeatHost_.toString() << "/" << heartBeatPort_
		     << " - " << heartBeatWriter_->errorString() << std::endl;
	}
    }
}

void
MulticastMessageReader::socketReadyRead()
{
    static const qint64 kMaxSize = 64 * 1024 - 1;
    static Logger::ProcLog log("socketReadyRead", Log());
    LOGINFO << std::endl;

    ACE_Message_Block* raw = 0;
    do {

	qint64 size = proxy_->pendingDatagramSize();
	if (size == -1)
	    continue;

	if (size > kMaxSize) {
	    LOGERROR << this << " invalid size from pendingDatagramSize - "
		     << size << std::endl;
	    continue;
	}

	if (! raw || raw->size() < size_t(size)) {
	    if (raw) raw->release();
	    raw = MessageManager::MakeMessageBlock(size);
	}

	QHostAddress host;
	quint16 port;
	size = proxy_->readDatagram(raw->wr_ptr(), size, &host, &port);
	if (size == -1) {
	    LOGERROR << "failed to fetch data" << std::endl;
	    continue;
	}

	if (port_ != port) {
	    LOGERROR << "wrong port value: " << port_ << " != " << port
		     << std::endl;
	    continue;
	}

	if (! connected_) {
	    connected_ = true;
	    emit connected();
	}

	raw->wr_ptr(size);
	addRawData(raw);
	raw = 0;

    } while (proxy_->hasPendingDatagrams());

    if (raw)
	raw->release();
}

static const char*
getSocketErrorText(QAbstractSocket::SocketError err)
{
    switch (err) {
    case QAbstractSocket::ConnectionRefusedError:
	return "connection refused";
    case QAbstractSocket::RemoteHostClosedError:
	return "remote host closed connection";
    case QAbstractSocket::HostNotFoundError:
	return "host address not found";
    case QAbstractSocket::SocketAccessError:
	return "socket operation failed due to access restrictions";
    case QAbstractSocket::SocketResourceError:
	return "local system ran out of resources";
    case QAbstractSocket::SocketTimeoutError:
	return "socket operation timed out";
    case QAbstractSocket::DatagramTooLargeError:
	return "datagram was too big";
    case QAbstractSocket::NetworkError:
	return "network error (check cable)";
    case QAbstractSocket::AddressInUseError:
	return "address already in use and exclusive";
    case QAbstractSocket::SocketAddressNotAvailableError:
	return "address does not belong to host";
    case QAbstractSocket::UnsupportedSocketOperationError:
	return "unsupported socket operation";
    case QAbstractSocket::UnknownSocketError:
    default:
	return "unknown socket error";
    }
}

void
MulticastMessageReader::socketError(QAbstractSocket::SocketError err)
{
    static Logger::ProcLog log("socketError", Log());
    LOGERROR << "err: " << err << " - " << getSocketErrorText(err)
	     << std::endl;
}

void
MulticastMessageReader::beatHeart()
{
    sendHeartBeat("HI");
}
