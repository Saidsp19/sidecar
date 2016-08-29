#ifndef SIDECAR_GUI_WRITERS_H // -*- C++ -*-
#define SIDECAR_GUI_WRITERS_H

#include "boost/shared_ptr.hpp"

#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtCore/QString"
#include "QtCore/QTime"
#include "QtCore/QTimer"
#include "QtNetwork/QUdpSocket"

#include "IO/Writers.h"
#include "IO/ZeroconfRegistry.h"

class QAbstractSocket;
class QTcpServer;
class QTcpSocket;

namespace Logger { class Log; }

namespace SideCar {
namespace Zeroconf { class Publisher; }
namespace GUI {

class ServiceBrowser;
class ServiceEntry;

/** SideCar data network writer that uses a QAbstractSocket to to the actual writing. Conforms to the interface
    required by the IO::TWriter template. Allows QAbstractSocket objects to be used in the SideCar IO framework.
*/
class SocketWriterDevice
{
public:

    /** Constructor.
     */
    SocketWriterDevice() : device_(0) {}

    /** Destructor. Closes and delete any managed QAbstractSocket.
     */
    ~SocketWriterDevice();

    /** Assign a Qt socket to the writer. Assumes ownership of the socket.

        \param device the Qt socket to use for writing
    */
    void setDevice(QAbstractSocket* device);

    /** Obtain the assigned Qt socket.

        \return QAbstractSocket object
    */
    QAbstractSocket* getDevice() const { return device_; }

protected:
    
    /** Implementation of IO::TWriter API that performs writes to the network device.

        \param iov pointer to Unix iovec structure that contains data pointers and sizes to write

        \param count number of iovec entries to process

        \return total number of bytes written to the device, or -1 if error
    */
    ssize_t writeToDevice(const iovec* iov, int count);

private:
    QAbstractSocket* device_;
};

/** Derivation of an IO::TWriter class that uses QAbstractSocket objects to perform data writes.
 */
class SocketWriter : public IO::TWriter<SocketWriterDevice>
{
public:
    using Super = IO::TWriter<SocketWriterDevice>;

    SocketWriter() : Super() {}
};

/** Abstract base class for Qt data publishers. Manages a Zeroconf::Publisher to broadcast publishing
    information. Derived classes must implement the writeMessage() method.
*/
class MessageWriter : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    /** Obtain the Log device to use for MessageWriter objects

        \return Log device
    */
    static Logger::Log& Log();

    ~MessageWriter();
    
    /** Set the service name to publish under

        \param serviceName the name to publish
    */
    void setServiceName(const QString& serviceName);

    /** Obtain the service name being published.

        \return service name
    */
    const QString& getServiceName() const { return serviceName_; }

    /** Obtain the multicast address to use for sending out data

        \return IPV4 multicast address
    */
    const QString& getServiceAddress() const { return serviceAddress_; }

    /** Obtain the socket port number used by the publisher

        \return socket port number
    */
    uint16_t getPort() const { return port_; }

    /** Start the publisher.

        \return true if successful
    */
    virtual bool start();

    /** Publish a SideCar data message. Derived classes must define.

        \param mgr container holding the message to write

        \return true if successful
    */
    virtual bool writeMessage(const IO::MessageManager& mgr) = 0;

    /** Stop the publisher.

        \return true if successful
    */
    virtual bool stop();

    /** Determine if the MessageWriter is currently publishing its connection info

        \return true if publishing
    */
    bool isPublished() const;

signals:

    /** Notification sent out when the MessageWriter successfully publishes its connection information.

        \param serviceName name that was published

        \param serviceAddress host address that was published

        \param port socket port that was published
    */
    void published(const QString& serviceName, const QString& serviceAddress, uint16_t port);

    /** Notification sent out when the MessageWriter encounters a failure. Currently this is only sent out if
	publishing fails.
    */
    void failure();

    /** Notification sent out when the number of subscribers to the publisher changes. Note that this class does
        not emit this signal; derived classes are expected to when they determine that the count has changed.

        \param count new subscriber count
    */
    void subscriberCountChanged(size_t count);

protected:

    /** Constructor. Restricted to derived classes.

        \param serviceName name of the service to publish

        \param type name of the message type to publish

        \param transport the IP transport to use ('tcp' or 'multicast').

        \param serviceAddress the address to publish for the server. Normally this would be the host IP address or
        name, but for multicasting, this must be the multicast address to use.
    */
    MessageWriter(const QString& serviceName, const std::string& type, const std::string& transport,
                  const QString& serviceAddress);

    /** Set the socket port assigned to the publisher by the OS.

        \param port socket port to use for writing
    */
    void setPort(uint16_t port) { port_ = port; }

    /** Update the service name to the one created by Zeroconf due to a name collision.

        \param serviceName new service name
    */
    virtual void setPublishedServiceName(const QString& serviceName);

    /** Obtain the Zeroconf::Publisher object used to broadcast publisher connection information.

        \return Zeroconf::Publisher reference
    */
    boost::shared_ptr<Zeroconf::Publisher> getPublisher() const { return publisher_; }

private:

    /** Notification from Zeroconf::Publisher that the publish action has finished.
        
        \param state true if the publishing was successful, false otherwise.
    */
    void publishedNotification(bool state);

    QString serviceName_;
    std::string type_;
    std::string transport_;
    QString serviceAddress_;
    boost::shared_ptr<Zeroconf::Publisher> publisher_;
    uint16_t port_;
};

/** Qt message writer that uses QTcpServer/QTcpSocket to write data over a TCP connection. Acts as a
    garden-variety TCP server, accepting socket connections, and sending data over each connection.

    NOTE: this is a simplistic implementation that does not scale very well. For publishers serving a large number of
    connections, one should use UDPMessageWriter, which uses multicasting to emit messages.

    Uses QTcpServer to manage socket connections. Each connection to a subscriber has its own QTcpSocket instance.
*/
class TCPMessageWriter : public MessageWriter, public IO::ZeroconfTypes::Publisher
{
    Q_OBJECT
    using Super = MessageWriter;
public:

    /** Obtain the log device to use for TCPMessageWriter objects
        
        \return Log device
    */
    static Logger::Log& Log();

    /** Factory method used to creates a new, initialized TCPMessageWriter.
        
        \param serviceName name to publish under

        \param subType message type of the messages to write

        \return new TCPMessageWriter if successful, NULL otherwise
    */
    static TCPMessageWriter* Make(const QString& serviceName, const std::string& subType);

    /** Start the QTcpServer and publish connection information.
        
        \return true if successful
    */
    bool start();

    /** Implementation of the MessageWriter API. Sends the message held by the given IO::MessageManager to each
        TCP connection.

        \param mgr container holding the message to write

        \return true if successful
    */
    bool writeMessage(const IO::MessageManager& mgr);

    bool writeEncoded(ACE_Message_Block* data);

    /** Stop the publisher and QTcpServer. Closes all active client QTcpSocket connections.

        \return true if successful
    */
    bool stop();

private slots:

    /** Notification handler when a subscriber / client requests to connect.
     */
    void newSubscriberConnection();

    /** Notification handler when a subscriber / client closes its connection.
     */
    void subscriberDisconnected();

private:

    /** Constructor. Prohibited. Use the Make() factory method to create new instances.
        
        \param serviceName name to publish under

        \param subType message type of the messages to write
    */
    TCPMessageWriter(const QString& serviceName, const std::string& subType);

    QTcpServer* server_;
    QList<SocketWriter*> subscribers_;
};

using ServiceEntryList = QList<ServiceEntry*>;

/** Qt message writer that uses IO::UDPSocketWriter to do the writing. Unlike TCPMessageWriter, there is nothing
    gained by using a Qt QUdpSocket to handle the data writing.
*/
class UDPMessageWriter : public MessageWriter, public IO::ZeroconfTypes::Publisher
{
    Q_OBJECT
    using Super = MessageWriter;
public:

    /** Obtain the log device to use for objects of this class
        
        \return Logger::Log reference
    */
    static Logger::Log& Log();

    /** Factory method used to create a new, initialized UDPMessageWriter object.
        
        \param serviceName name to publish under

        \param subType type of the message to write

        \param address multicast address to use

	\param port optional port value to use

        \return 
    */
    static UDPMessageWriter* Make(const QString& serviceName, const std::string& subType,
                                  const QString& multicastAddress, uint16_t port = 0);

    /** Implementation of the MessageWriter API. Sends the message held by the given IO::MessageManager out onto
        the network with the configured multicast address.
        
        \param mgr container holding the message to write

        \return true if successful
    */
    bool writeMessage(const IO::MessageManager& mgr);

    bool writeEncoded(ACE_Message_Block* data);

private slots:

    /** Notification that there is a heart-beat message available for reading.
     */
    void heartBeatReady();

    /** Routine periodically invoked by QTimer object to look for stale heart-beat entries in the heartBeats_
        map.
    */
    void checkHeartBeats();

private:

    /** Constructor. Prohibited. Use the Make() factory method to create new instances.

        \param serviceName name to publish under

        \param subType type of the messages to handle

        \param address multicast address to use
    */
    UDPMessageWriter(const QString& serviceName, const std::string& subType, const QString& multicastAddress);

    /** Initialize the socket, setting its destination address, multicast TTL, and assigned port number.

        \param addr multicast address to use

        \return true if successful
    */
    bool initialize(uint16_t port);

    IO::UDPSocketWriter writer_;
    QUdpSocket* heartBeatReader_;
    using HeartBeatMap = std::map<QString,QTime>;
    HeartBeatMap heartBeats_;
    size_t active_;
    QTimer* timer_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
