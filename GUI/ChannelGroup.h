#ifndef SIDECAR_GUI_CHANNELGROUP_H // -*- C++ -*-
#define SIDECAR_GUI_CHANNELGROUP_H

#include "QtCore/QObject"

#include "GUI/ServiceBrowser.h"
#include "IO/ZeroconfRegistry.h"

class QComboBox;

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class MessageList;
class Subscriber;

/** A manager of a collection of subscription channels for a particular message type (eg. Video or BinaryVideo
    messages). Contains a ServiceBrowser which monitors Zeroconfig announcements from publishers, and a
    QComboBox widget that contains the names of the publishers available for selection. Finally, it also
    contains a ReaderThread that will subscribe to a chosen publisher and read in the published data.

    Originally, this was a part of the PPIDisplay program, but was hoisted to the GUI directory with hopes that
    it would be useful in other programs. That has not proved the case; it should go back to PPIDisplay.
*/
class ChannelGroup : public QObject, public IO::ZeroconfTypes::Subscriber
{
    Q_OBJECT
public:

    /** Log device for objects of this class

        \return log device
    */
    static Logger::Log& Log();
    
    /** Constructor.

        \param parent owner of this object (for destruction)

        \param typeName message type of the publishers to watch for

        \param found the widget to update with publisher changes
    */
    ChannelGroup(QObject* parent, const std::string& typeName, QComboBox* found);

signals:
    
    /** Notification sent out when the channel has data.

        \param messages list of Messages::Header::Ref objects
    */
    void incoming(const MessageList& messages);

    /** Notification that the user requested a new channel to connect to.
     */
    void channelChanged();

public slots:

    /** Shutdown the reader thread and its connection to a publisher.
     */
    void shutdown();

private slots:

    /** Update the QComboBox with a new set of publisher names.

        \param services list of available publishers
    */
    void setAvailableServices(const ServiceEntryHash& services);

    /** Process a change in the selected item of the QComboBox. Attempts to resolve the publsher's connection
        information.

        \param index index of the new selection
    */
    void selectionChanged(int index);

    /** Notification from the Subscriber object that it has data available for fetching.
     */
    void dataAvailable();

private:
    
    /** Change to the channel indicated by the given index

        \param index the channel to use
    */
    void changeChannels(int index);

    std::string typeName_;
    ServiceBrowser* browser_;
    ServiceEntry* activeServiceEntry_;
    QComboBox* found_;
    Subscriber* subscriber_;
    QString settingsKey_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
