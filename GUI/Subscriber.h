#ifndef SIDECAR_GUI_SUBSCRIBER_H // -*- C++ -*-
#define SIDECAR_GUI_SUBSCRIBER_H

#include "QtCore/QObject"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class MessageList;
class ReaderThread;
class ServiceEntry;

/** The input side of a publisher/subscriber pair. Manages a separate thread that hosts a TCP/UDP reader which
    obtains data from the publisher. Uses the incoming() signal to announce the arrival of new messages.

    The useServiceEntry() method gives the subscriber connection information of
    a publichser to connect to.
*/
class Subscriber : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    static Logger::Log& Log();

    /** Constructor.

        \param parent owner for the purposes of destruction via Qt hierarchy
    */
    Subscriber(QObject* parent = 0);

    /** Destructor. Terminates any active reader thread.
     */
    ~Subscriber();

    /** Set publisher connection info to use. May be NULL which will close any active connection.

        \param serviceEntry data publisher connection info
    */
    void useServiceEntry(ServiceEntry* serviceEntry);

    /** Obtain the connection state of the subscriber.

        \return true if connected
    */
    bool isConnected() const { return connected_; }

    /** Obtain the set of messages available from the publisher. May be NULL if there is no connection to a
        publisher.

        \return list of Messages::Header::Ref references
    */
    const MessageList& getMessages() const;

signals:

    /** Notification that a connection has been established to a publisher.
     */
    void connected();

    /** Notification that one or more messages are available to fetch via the getMessages().
     */
    void dataAvailable();

    /** Notification that a connection has been dropped.
     */
    void disconnected();

public slots:

    /** Shut down any active connection to a publisher.
     */
    void shutdown();

private slots:

    /** Notification from a ServiceEntry that it has resolved its host/port information.

        \param serviceEntry object that resolved
    */
    void resolvedServiceEntry(ServiceEntry* serviceEntry);

    /** Notification from a ServiceEntry that it is about to be deleted.
     */
    void lostServiceEntry();

    /** Notification from the internal ReaderThread object that it has established a connection to the
	publisher. For UDP connections, this signal comes when the first message arrives over the UDP socket.
    */
    void readerConnected();

    /** Notification from the internal ReaderThread object that it has lost its connection to the publisher.
	Only valid for TCP connections.
    */
    void readerDisconnected();

private:
    ServiceEntry* serviceEntry_; ///< Current connection information
    ReaderThread* reader_;	 ///< Separate thread that performs the reads
    bool connected_;		 ///< Connection state 
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
