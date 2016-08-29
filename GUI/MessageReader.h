#ifndef SIDECAR_GUI_MESSAGEREADER_H // -*- C++ -*-
#define SIDECAR_GUI_MESSAGEREADER_H

#include "QtCore/QObject"
#include "QtCore/QString"

#include "Messages/Header.h"
#include "Messages/MetaTypeInfo.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class ServiceEntry;

/** Base class for SideCar message readers used by the GUI applications. Contains no actual connection code; see
    derived classes TCPMessageReader and MulticastMessageReader.

    Manages two MessageList objects, one that is active for incoming data, and one that was active but can safely be
    used by another thread to obtain received data.
*/
class MessageReader : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    /** Obtain the log device for MessageReader objects

        \return Logger::Log reference
    */
    static Logger::Log& Log();

    /** Create a new MessageReader object using information from a publisher's ServiceEntry object.

        \param service connection description for a publisher

        \return new MessageReader object, or NULL if error
    */
    static MessageReader* Make(const ServiceEntry* service);

    /** Create a new MessageReader object using information from a publisher's ServiceEntry object.

        \param service connection description for a publisher

        \return new MessageReader object, or NULL if error
    */
    static MessageReader* Make(const std::string& topic, const Messages::MetaTypeInfo* type);

    /** Obtain the meta type info for the messages being received.

        \return Messages::MetaTypeInfo object
    */
    const Messages::MetaTypeInfo* getMetaTypeInfo() const { return metaTypeInfo_; }

signals:
    
    /** Notification that the reader has connected to a publisher
     */
    void connected();

    /** Notification that a new message is available from a publisher
     */
    void received(const Messages::Header::Ref& msg);

    /** Notification that the reader has disconnected from a publisher.
     */
    void disconnected();

protected:
    
    /** Constructor. Prohibits direct instantiation of this class. Use the class Make() methods instead.

	\param metaTypeInfo message type descriptor for data being read
    */
    MessageReader(const Messages::MetaTypeInfo* metaTypeInfo);

    /** Add raw incoming message data to the active message queue. First converts the raw data into a SideCar
        message type. Thread safe. Emits the dataAvailable() signal if this is the first message added he active
        message queue.

        \param raw pointer to raw message data
    */
    void addRawData(ACE_Message_Block* raw);

private:
    
    const Messages::MetaTypeInfo* metaTypeInfo_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
