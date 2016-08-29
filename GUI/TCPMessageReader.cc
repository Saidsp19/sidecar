#include "LogUtils.h"
#include "ServiceEntry.h"
#include "TCPMessageReader.h"

using namespace SideCar::IO;
using namespace SideCar::GUI;

Logger::Log&
TCPMessageReader::Log()
{
    static Logger::Log& log_ =
	Logger::Log::Find("SideCar.GUI.TCPMessageReader");
    return log_;
}

TCPMessageReader*
TCPMessageReader::Make(const ServiceEntry* service)
{
    static Logger::ProcLog log("Make", Log());
    QTcpSocket* socket = new QTcpSocket;
    socket->connectToHost(service->getHost(), service->getPort(),
                          QIODevice::ReadOnly);
    return new TCPMessageReader(service->getMetaTypeInfo(), socket);
}

TCPMessageReader::TCPMessageReader(const Messages::MetaTypeInfo* metaTypeInfo,
                                   QTcpSocket* socket)
    : Super(metaTypeInfo), reader_(), socket_(socket)
{
    reader_.setDevice(socket_);
    connect(socket, SIGNAL(connected()), this, SIGNAL(connected()));
    connect(socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
}

TCPMessageReader::~TCPMessageReader()
{
    delete socket_;
}

void
TCPMessageReader::socketReadyRead()
{
    static Logger::ProcLog log("socketReadyRead", Log());
    LOGINFO << std::endl;

    do {

	if (! reader_.fetchInput()) {
	    LOGERROR << "failed to fetch data" << std::endl;
	    break;
	}

	if (reader_.isMessageAvailable()) {
	    addRawData(reader_.getMessage());
	    LOGDEBUG << "got incoming message" << std::endl;
	}

    } while (socket_->bytesAvailable() > 0);
}
