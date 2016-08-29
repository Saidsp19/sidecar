#ifndef SIDECAR_GUI_READERTHREAD_H // -*- C++ -*-
#define SIDECAR_GUI_READERTHREAD_H

#include "boost/scoped_ptr.hpp"

#include "QtCore/QMutex"
#include "QtCore/QString"
#include "QtCore/QThread"

#include "GUI/MessageList.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Messages { class MetaTypeInfo; }
namespace GUI {

class ServiceEntry;

/** A class that manages a TCP/UDP reader that runs in a separate thread. Implements the signaling interface
    defined by the GUI::MessageReader class. A new thread starts when useServiceEntry() is called with a valid
    ServiceEntry object (called with a NULL object will stop any running thread). The thread runs the run()
    method, which creates a new MessageReader object, hooks up its signals to the reader* slots below, and then
    starts up a new Qt event loop.
*/
class ReaderThread : public QThread
{
    Q_OBJECT
    using Super = QThread;
public:

    static Logger::Log& Log();
    
    /** Constructor. Initializes state. NOTE: changes thread ownership from the main thread to the this QThead
        object. Any signal/slot connections to this object will be handled by queued events.
    */
    ReaderThread();

    /** Destructor. Stops the thread if it is still running.
     */
    ~ReaderThread();

    /** Use the connection information from a ServiceEntry object. Stops any active thread, and starts a new
        one.

        \param serviceEntry object containing the resolved connection
        information for the MessageReader to use
    */
    void useServiceEntry(const ServiceEntry* serviceEntry);

    /** Obtain the Zeroconf sub-type last assigned in useServiceEntry()

        \return sub-type value
    */
    const Messages::MetaTypeInfo* getMetaTypeInfo() const
	{ return metaTypeInfo_; }

    /** Obtain the list of data messages received from the internal MessageReader object.
     */
    const MessageList& getMessages();

public slots:

    /** If a thread is running, stop it and wait for it to exit.
     */
    void stop();

private slots:

    void received(const Messages::Header::Ref& msg);

signals:

    /** Notification sent out when the internal MessageReader object emits its connected signal.
     */
    void connected();

    /** Notification sent out when the internal MessageReader object emits its incoming signal.
     */
    void dataAvailable();

    /** Notification sent out when the internal MessageReader object emits its disconnected signal.
     */
    void disconnected();

protected:
    
    /** Method run in a separate process. Implements QThread interface. Creates a new MessageReader object to do
	the reading, and starts a new Qt event loop. The thread runs until stopped by the stop() method.
    */
    void run();

private:
    boost::scoped_ptr<ServiceEntry> serviceEntry_;
    const Messages::MetaTypeInfo* metaTypeInfo_;
    MessageList messages_[2];
    volatile size_t active_;
    QMutex mutex_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
