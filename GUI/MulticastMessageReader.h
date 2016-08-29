#ifndef SIDECAR_GUI_MULTICASTMESSAGEREADER_H // -*- C++ -*-
#define SIDECAR_GUI_MULTICASTMESSAGEREADER_H

#include "QtCore/QTimer"
#include "QtNetwork/QUdpSocket"

#include "GUI/MessageReader.h"
#include "IO/Readers.h"
#include "IO/ZeroconfRegistry.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Zeroconf { class Publisher; }
namespace GUI {

/** Derivation of MessageReader that obtains messages over a UDP multicast socket. Use the Make() class method
    to create new instances, or even better use the MessageReader::Make() methods.

    Creates a QUdpSocket to serve as a proxy for the real socket that was
    created, and set up by the IO::MulticastSocketReader class. When data is
    available to read from the socket, the socketReadyRead() method runs and
    puts complete datagrams into new ACE_Message_Block objects, which the
    method hands off to the MessageReader::addRawData() method to add to the
    active message list.
*/
class MulticastMessageReader : public MessageReader,
			       public IO::ZeroconfTypes::Subscriber
{
    Q_OBJECT
    using Super = MessageReader;
public:
    using ZeroconfPublisherRef = boost::shared_ptr<Zeroconf::Publisher>;

    /** Obtain the log device for MulticastMessageReader objects.

        \return log device
    */
    static Logger::Log& Log();

    /** Factory class method that creates new MulticastMessageReader objects.

        \param type the message type for data coming in from the publisher

        \param publisherName the name of the publisher sending the data

        \param publisherHost the host where the publisher resides

        \param publisherPort the port over which the publisher sends the data

        \return new MulticastMessageReader object, or NULL if error
    */
    static MulticastMessageReader* Make(const ServiceEntry* service);

    /** Destructor.
     */
    ~MulticastMessageReader();

private slots:

    /** Notification from QUdpSocket that datagrams are available.
     */
    void socketReadyRead();

    /** Notification from QUdpSocket that an error has occured. Shuts down the socket connection.

        \param err error descriptor
    */
    void socketError(QAbstractSocket::SocketError err);

    /** Timer event handler that sends a heart-beat message to our publisher.
     */
    void beatHeart();

private:

    /** Constructor.
     */
    MulticastMessageReader(const Messages::MetaTypeInfo* metaTypeInfo);

    /** Attempt to open a UDP socket to receive data from the given host/port.

        \param host 

        \param port 

        \return 
    */
    bool open(const ServiceEntry* service);

    /** Emit a heart-beat message to our publisher. Supported messages are "HI" and "BYE". NOTE: this should be
        changed to an enumerated message type in a separate message class. However, since we are moving to DDS,
        this will become obsolete anyway.

        \param msg 
    */
    void sendHeartBeat(const char* msg);

    quint16 port_;
    QUdpSocket* proxy_;
    QHostAddress heartBeatHost_;
    quint16 heartBeatPort_;
    QUdpSocket* heartBeatWriter_;
    QTimer* timer_;
    IO::MulticastSocketReader reader_;
    bool connected_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
