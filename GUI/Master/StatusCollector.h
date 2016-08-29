#ifndef SIDECAR_GUI_MASTER_STATUSCOLLECTOR_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_STATUSCOLLECTOR_H

#include "boost/shared_ptr.hpp"

#include "QtCore/QByteArray"
#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtNetwork/QUdpSocket"

#include "Runner/StatusEmitter.h"

class QUdpSocket;

namespace Logger { class Log; }

namespace SideCar {
namespace Zeroconf { class Publisher; }
namespace GUI {
namespace Master {

/** Status collector for Runner StatusEmitter objects. Accepts XML status information from a StatusEmitter that
    details the status of the IO::Stream and IO::Task objects owned by the Runner. Hands off incoming status
    data to a ServiceModel object via ServicesModel::updateStatus() method.
*/
class StatusCollector : public QObject
{
    Q_OBJECT
public:

    using ZCPublisherRef = boost::shared_ptr<Zeroconf::Publisher>;

    /** Log device for all StatusCollector objects

        \return log device
    */
    static Logger::Log& Log();

    /** Obtain the Zeroconf type for all Runner StatusEmitter objects associated with this StatusCollector
	class.

	\return NULL-terminated C string
    */
    static const char* GetCollectorType()
	{ return Runner::StatusEmitter::GetCollectorType(); }
    
    /** Constructor.

        \param model the object to notify when new status arrives.
    */
    StatusCollector();

    /** Creates a UDP socket to use to receive status data, and then publishes the UDP port number for
        StatusEmitters to use to when sending status data.

        \return true if successful
    */
    bool open();

    /** Shutdown the Zeroconf publisher, and close the UDP socket.
     */
    void close();

signals:

    /** Signal sent out whenever a new status message is received.
     */
    void statusUpdates(const QList<QByteArray>& statusUpdates);

private slots:

    /** Notification from the UDP socket that data is available.
     */
    void dataAvailable();

    /** Notification from the UDP socket that an error has occurred.

        \param err 
    */
    void error(QAbstractSocket::SocketError err);

private:
    ZCPublisherRef publisher_;
    QUdpSocket* socket_;
};

} // end namespace Master
} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
