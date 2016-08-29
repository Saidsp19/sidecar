#ifndef SIDECAR_GUI_CHANNELSETTING_H // -*- C++ -*-
#define SIDECAR_GUI_CHANNELSETTING_H

class QComboBox;

#include "GUI/ServiceBrowser.h"
#include "GUI/StringSetting.h"
#include "IO/ZeroconfRegistry.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class MessageList;
class Subscriber;

/** Derivation of StringSetting that works with a QComboBox widget to display and change a subscription channel
    setting. Uses the QComboBox::currentIndexChanged() signal to detect changes in the widget.
*/
class ChannelSetting : public StringSetting,
		       public IO::ZeroconfTypes::Subscriber
{
    Q_OBJECT
    using Super = StringSetting;
public:

    /** Log device for objects of this class

        \return log device
    */
    static Logger::Log& Log();

    static QString GetChannelName(const std::string& typeName);

    ChannelSetting(const std::string& typeName, PresetManager* mgr,
                   QComboBox* widget = 0, bool global = false);

    void connectWidget(QComboBox* widget);

    bool hasChannels() const;

    bool isConnected() const;

signals:

    /** Notification sent out when the channel has data.

        \param messages list of Messages::Header::Ref objects
    */
    void incoming(const MessageList& messages);

    void availableChannels(const QList<QString>& names);

    void valueChanged(int index);

    void channelsAvailable(bool value);

public slots:

    void shutdown();

private slots:

    /** Update the QComboBox with a new set of publisher names.

        \param services list of available publishers
    */
    void setAvailableServices(const ServiceEntryHash& services);

    /** Notification from the Subscriber object that it has data available for fetching.
     */
    void dataAvailable();

private:

    /** Override of StringSetting::valueUpdated() method. Records the new value and emits the valueChanged(int
	) and valueChanged(QString) signals.
    */
    void valueUpdated();

    /** Change to the channel indicated by the given index

        \param index the channel to use
    */
    void changeChannels(int index);

    ServiceBrowser* browser_;
    ServiceEntry* activeServiceEntry_;
    QComboBox* first_;
    Subscriber* subscriber_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
