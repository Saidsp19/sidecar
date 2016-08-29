#include "ace/ACE.h"
#include "ace/INET_Addr.h"
#include "boost/bind.hpp"

#include "Zeroconf/Publisher.h"	// !!! Place BEFORE any Qt headers !!!

#include "Utils/Format.h"

#include "QtNetwork/QHostInfo"
#include "QtNetwork/QTcpServer"
#include "QtNetwork/QTcpSocket"
#include "QtNetwork/QUdpSocket"

#include "GUI/ServiceBrowser.h"
#include "IO/MessageManager.h"

#include "LogUtils.h"
#include "QtMonitor.h"
#include "ServiceEntry.h"
#include "Writers.h"

using namespace SideCar::GUI;

SocketWriterDevice::~SocketWriterDevice()
{
    if (device_) {
	device_->close();
	device_->deleteLater();
    }
}

void
SocketWriterDevice::setDevice(QAbstractSocket* device)
{
    if (device_ != device) {
	if (device_) device_->deleteLater();
	device_ = device;
    }
}

ssize_t
SocketWriterDevice::writeToDevice(const iovec* iov, int count)
{
    return ACE::sendv(device_->socketDescriptor(), iov, count);
}

Logger::Log&
MessageWriter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.MessageWriter");
    return log_;
}

MessageWriter::MessageWriter(const QString& serviceName, const std::string& type, const std::string& transport,
                             const QString& serviceAddress)
    : serviceName_(serviceName), type_(type), transport_(transport), serviceAddress_(serviceAddress),
      publisher_(Zeroconf::Publisher::Make(new QtMonitor)), port_(0)
{
    static Logger::ProcLog log("MessageWriter", Log());
    LOGINFO << "serviceName: " << serviceName << " type: " << type << " serviceAddress: " << serviceAddress
            << std::endl;
    publisher_->connectToPublishedSignal(boost::bind(&MessageWriter::publishedNotification, this, _1));
}

MessageWriter::~MessageWriter()
{
    static Logger::ProcLog log("~MessageWriter", Log());
    LOGINFO << std::endl;
    publisher_->stop();
}

bool
MessageWriter::isPublished() const
{
    return publisher_->isPublished();
}

void
MessageWriter::setServiceName(const QString& serviceName)
{
    static Logger::ProcLog log("setServiceName", Log());
    LOGINFO << "serviceName: " << serviceName << std::endl;
    if (serviceName != serviceName_) {
        bool publishing = isPublished();
	if (publishing) {
            stop();
        }
        
	serviceName_ = serviceName;
	if (publishing) {
            start();
        }
    }
}

void
MessageWriter::setPublishedServiceName(const QString& serviceName)
{
    serviceName_ = serviceName;
}

bool
MessageWriter::start()
{
    static Logger::ProcLog log("start", Log());
    LOGINFO << serviceName_ << std::endl;
    publisher_->setType(type_);
    publisher_->setHost(serviceAddress_.toStdString());
    publisher_->setPort(port_);
    publisher_->setTextData("transport", transport_, false);
    if (! publisher_->publish(serviceName_.toStdString(), false)) {
	LOGERROR << "failed to publish" << std::endl;
	return false;
    }

    return true;
}

bool
MessageWriter::stop()
{
    static Logger::ProcLog log("stop", Log());
    LOGINFO << std::endl;
    publisher_->stop();
    return true;
}

void
MessageWriter::publishedNotification(bool state)
{
    static Logger::ProcLog log("publishedNotification", Log());
    QString publishedName(QString::fromStdString(publisher_->getName()));
    LOGINFO << "state: " << state << " publishedName: " << publishedName << " port: " << port_ << std::endl;

    if (state) {
	if (publishedName != serviceName_)
	    LOGWARNING << "publishedName '" << publishedName << "' is not the same as requested serviceName: '"
		       << serviceName_ << "'" << std::endl;
	setPublishedServiceName(publishedName);
	emit published(publishedName, serviceAddress_, port_);
    }
    else {
	emit failure();
    }
}

Logger::Log&
TCPMessageWriter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.TCPMessageWriter");
    return log_;
}

TCPMessageWriter*
TCPMessageWriter::Make(const QString& serviceName, const std::string& subType)
{
    static Logger::ProcLog log("Make", Log());
    TCPMessageWriter* writer = new TCPMessageWriter(serviceName, subType);
    if (! writer->start()) {
	LOGERROR << "failed to start server" << std::endl;
	delete writer;
	writer = 0;
    }

    return writer;
}

TCPMessageWriter::TCPMessageWriter(const QString& serviceName, const std::string& subType)
    : Super(serviceName, MakeZeroconfType(subType), "tcp", QHostInfo::localHostName()), server_(0), subscribers_()
{
    static Logger::ProcLog log("TCPMessageWriter", Log());
    LOGINFO << std::endl;
}

bool
TCPMessageWriter::start()
{
    static Logger::ProcLog log("start", Log());
    LOGINFO << std::endl;

    server_ = new QTcpServer(this);
    connect(server_, SIGNAL(newConnection()), SLOT(newSubscriberConnection()));

    if (! server_->listen(QHostAddress::Any, 0)) {
	LOGERROR << "failed to start server" << std::endl;
	delete server_;
	server_ = 0;
	return false;
    }

    setPort(server_->serverPort());

    return Super::start();
}

bool
TCPMessageWriter::stop()
{
    static Logger::ProcLog log("stop", Log());
    LOGINFO << std::endl;
    if (server_) {
	server_->close();
	delete server_;
	server_ = 0;
    }

    while (subscribers_.size()) {
	delete subscribers_.takeAt(0);
    }

    emit subscriberCountChanged(subscribers_.size());

    return Super::stop();
}

void
TCPMessageWriter::newSubscriberConnection()
{
    static Logger::ProcLog log("newSubscriberConnection", Log());
    QTcpSocket* socket = server_->nextPendingConnection();
    LOGINFO << socket << std::endl;

    if (! socket) return;

    connect(socket, SIGNAL(disconnected()), SLOT(subscriberDisconnected()));

    SocketWriter* subscriber = new SocketWriter;
    subscriber->setDevice(socket);
    subscribers_.append(subscriber);

    emit subscriberCountChanged(subscribers_.size());
}

void
TCPMessageWriter::subscriberDisconnected()
{
    static Logger::ProcLog log("subscriberDisconnected", Log());
    LOGINFO << sender() << std::endl;
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    for (int index = 0; index < subscribers_.size(); ++index) {
	if (subscribers_[index]->getDevice() == socket) {
	    LOGDEBUG << "found at index " << index << std::endl;
	    SocketWriter* subscriber = subscribers_.takeAt(index);
	    delete subscriber;
	    emit subscriberCountChanged(subscribers_.size());
	    break;
	}
    }
}

bool
TCPMessageWriter::writeMessage(const IO::MessageManager& mgr)
{
    static Logger::ProcLog log("writeMessage", Log());
    LOGINFO << std::endl;

    QList<SocketWriter*>::iterator pos = subscribers_.begin();
    while (pos != subscribers_.end()) {
	SocketWriter* subscriber = *pos;
	if (! subscriber->write(mgr)) {
	    LOGWARNING << "failed to write to socket " << subscriber->getDevice() << std::endl;
	}
	++pos;
    }

    return true;
}

bool
TCPMessageWriter::writeEncoded(ACE_Message_Block* data)
{
    static Logger::ProcLog log("writeMessage", Log());
    LOGINFO << std::endl;

    QList<SocketWriter*>::iterator pos = subscribers_.begin();
    while (pos != subscribers_.end()) {
	SocketWriter* subscriber = *pos;
	ACE_Message_Block* tmp = data->duplicate();
	if (! subscriber->writeEncoded(1, tmp)) {
	    tmp->release();
	    LOGWARNING << "failed to write to socket " << subscriber->getDevice() << std::endl;
	}
	++pos;
    }

    return true;
}

Logger::Log&
UDPMessageWriter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.GUI.UDPMessageWriter");
    return log_;
}

UDPMessageWriter*
UDPMessageWriter::Make(const QString& serviceName, const std::string& subType, const QString& multicastAddress,
                       uint16_t port)
{
    static Logger::ProcLog log("Make", Log());
    LOGINFO << "serviceName: " << serviceName << " subType: " << subType << "multicast address: "
            << multicastAddress << std::endl;

    UDPMessageWriter* writer = new UDPMessageWriter(serviceName, subType, multicastAddress);
    if (! writer->initialize(port)) {
	LOGERROR << "failed to connect to multicast address " << multicastAddress << " - " << Utils::showErrno()
                 << std::endl;
	delete writer;
	return 0;
    }

    if (! writer->start()) {
	LOGERROR << "failed to start server" << std::endl;
	delete writer;
	return 0;
    }

    return writer;
}

UDPMessageWriter::UDPMessageWriter(const QString& serviceName, const std::string& subType,
                                   const QString& multicastAddress)
    : Super(serviceName, MakeZeroconfType(subType), "multicast", multicastAddress),
      writer_(), heartBeatReader_(0), heartBeats_(), active_(0), timer_(0)
{
    ;
}

bool
UDPMessageWriter::initialize(uint16_t port)
{
    static Logger::ProcLog log("initialize", Log());
    LOGINFO << "address: " << getServiceAddress() << std::endl;

    ACE_SOCK_Dgram& device(writer_.getDevice());

    // Open the writer device to use to send out data. For opening, we use the "any" address with a zero port.
    // The system will assign us a port number to use.
    //
    ACE_INET_Addr address(u_short(port), "0.0.0.0", AF_INET);
    LOGDEBUG << "opening UDP socket with address " << Utils::INETAddrToString(address) << std::endl;
    if (device.open(address, AF_INET, 0, 1) == -1) {
	LOGERROR << "failed to open multicast writer at " << getServiceAddress() << std::endl;
	return false;
    }

    LOGDEBUG << "opened socket " << writer_.getDevice().get_handle() << std::endl;

    // Set the hop time-to-live to a non-zero value so we don't flood the network.
    //
    unsigned char ttl = 10;
    if (device.set_option(IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
	LOGERROR << "failed to set TTL for " << getServiceAddress() << std::endl;
	return false;
    }

    if (! port) {

	// Fetch the port assigned to us by the system.
	//
	device.get_local_addr(address);
	port = address.get_port_number();
    }

    LOGDEBUG << "socket device address: " << Utils::INETAddrToString(address) << std::endl;

    // Set our Zeroconf publisher connection info.
    //
    setPort(port);

    // Build our multicast address now that we know our port number. Tell the
    // writer where to send the UDP messages.
    //
    address.set(port, getServiceAddress().toStdString().c_str(), 1, AF_INET);
    LOGDEBUG << "mcast address: " << Utils::INETAddrToString(address) << std::endl;
    writer_.setRemoteAddress(address);

    // Create a heart-beat message reader socket for remote client to send
    // their heart-beat messages to
    //
    heartBeatReader_ = new QUdpSocket(this);
    if (! heartBeatReader_->bind(0)) {
	LOGERROR << "failed to bind server socket" << std::endl;
	return false;
    }

    connect(heartBeatReader_, SIGNAL(readyRead()), SLOT(heartBeatReady()));

    quint16 heartBeatReaderPort = heartBeatReader_->localPort();
    LOGDEBUG << "heartBeatReader port: " << heartBeatReaderPort << std::endl;
    getPublisher()->setTextData("HeartBeatPort", QString::number(heartBeatReaderPort).toStdString());

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), SLOT(checkHeartBeats()));
    timer_->start(5000);

    return true;
}

void
UDPMessageWriter::heartBeatReady()
{
    static Logger::ProcLog log("heartBeatReady", Log());
    LOGINFO << heartBeatReader_ << std::endl;

    QByteArray data;
    data.resize(32);
    QHostAddress senderAddress;
    quint16 senderPort;
    bool notify = false;

    do {
	qint64 count = heartBeatReader_->readDatagram(data.data(), data.size(), &senderAddress, &senderPort);
	LOGDEBUG << "count: " << count << std::endl;

	if (count < 1) {
	    LOGERROR << "failed readDatagram" << std::endl;
	    return;
	}

	QString msg(data);
	LOGDEBUG << "msg: " << msg << std::endl;

	QString key = QString("%1/%2").arg(senderAddress.toString()).arg(senderPort);
	LOGDEBUG << "key: " << key << std::endl;

	HeartBeatMap::iterator pos = heartBeats_.find(key);
	if (msg == "HI") {
	    if (pos == heartBeats_.end()) {
		LOGDEBUG << "new entry" << std::endl;
		++active_;
		notify = true;
	    }
	    heartBeats_[key].start();
	}
	else if (msg == "BYE") {
	    HeartBeatMap::iterator pos = heartBeats_.find(key);
	    if (pos != heartBeats_.end()) {
		LOGDEBUG << "removing entry" << std::endl;
		heartBeats_.erase(pos);
		if (active_) {
		    --active_;
                }
		notify = true;
	    }
	}
    } while (heartBeatReader_->hasPendingDatagrams());

    if (notify) {
	emit subscriberCountChanged(active_);
    }
}

void
UDPMessageWriter::checkHeartBeats()
{
    static Logger::ProcLog log("checkHeartBeats", Log());
    static int kLimit = 30 * 1000; // 30 seconds in milliseconds

    if (! active_) return;

    bool notify = false;
    HeartBeatMap::iterator pos = heartBeats_.begin();
    HeartBeatMap::iterator end = heartBeats_.end();
    while (pos != end) {
	if (kLimit > pos->second.elapsed()) {
	    ++pos;
	}
	else {
	    HeartBeatMap::iterator tmp = pos++;
	    LOGDEBUG << "forgetting " << pos->first << std::endl;
	    heartBeats_.erase(tmp);
            if (active_) {
                --active_;
            }
	    notify = true;
	}
    }

    if (notify) {
	emit subscriberCountChanged(active_);
    }
}

bool
UDPMessageWriter::writeMessage(const IO::MessageManager& mgr)
{
    static Logger::ProcLog log("writeMessage", Log());
    LOGINFO << std::endl;
    return writer_.write(mgr);
}

bool
UDPMessageWriter::writeEncoded(ACE_Message_Block* data)
{
    static Logger::ProcLog log("writeEncoded", Log());
    LOGINFO << std::endl;
    return writer_.writeEncoded(1, data);
}

