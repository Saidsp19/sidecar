#include <errno.h>
#include <sstream>
#include <string>

#include "Zeroconf/Publisher.h" // NOTE: place before *any* Qt includes

#include "GUI/LogUtils.h"
#include "GUI/QtMonitor.h"
#include "Utils/Exception.h"

#include "StatusCollector.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

Logger::Log&
StatusCollector::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("master.StatusCollector");
    return log_;
}

StatusCollector::StatusCollector() : publisher_(Zeroconf::Publisher::Make(new QtMonitor)), socket_(new QUdpSocket(this))
{
    static Logger::ProcLog log("StatusCollector", Log());
    LOGINFO << std::endl;
    publisher_->setType(GetCollectorType());
    connect(socket_, SIGNAL(readyRead()), SLOT(dataAvailable()));
    connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(error(QAbstractSocket::SocketError)));
}

bool
StatusCollector::open()
{
    static Logger::ProcLog log("open", Log());

    // Using 0 gives us an unused port number, above 1024.
    //
    if (!socket_->bind(QHostAddress(QHostAddress::Any), 0)) {
        LOGFATAL << "failed to bind read socket - " << errno << ' ' << strerror(errno) << std::endl;
        return false;
    }

    LOGDEBUG << "local address: " << socket_->localAddress().toString() << '/' << socket_->localPort() << std::endl;

    LOGDEBUG << "peer address: " << socket_->peerAddress().toString() << '/' << socket_->peerPort() << std::endl;

    int port = socket_->localPort();
    publisher_->setPort(port);

    // Let Zeroconf deconflict our names. There is no harm in having multiple Master applications running, and
    // each one should receive status updates from running Runners.
    //
    std::ostringstream os;
    os << "StatusCollector-" << port;
    if (!publisher_->publish(os.str(), false)) {
        LOGFATAL << "failed to publish info - " << errno << ' ' << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

void
StatusCollector::close()
{
    static Logger::ProcLog log("close", Log());
    LOGINFO << std::endl;
    publisher_->stop();
    delete socket_;
}

void
StatusCollector::dataAvailable()
{
    static Logger::ProcLog log("dataAvailable", Log());

    QList<QByteArray> statusReports;

    do {
        qint64 size = socket_->pendingDatagramSize();
        LOGINFO << size << std::endl;
        if (size == -1) break;

        QByteArray buffer(size + 1, 0);
        size = socket_->readDatagram(buffer.data(), size);
        if (size == -1) break;

        statusReports.append(buffer);

    } while (socket_->hasPendingDatagrams());

    if (!statusReports.empty()) { emit statusUpdates(statusReports); }
}

void
StatusCollector::error(QAbstractSocket::SocketError err)
{
    static Logger::ProcLog log("error", Log());
    LOGINFO << err << std::endl;
}
