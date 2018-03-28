#ifndef SIDECAR_GUI_ASCOPE_VIDEOCHANNEL_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_VIDEOCHANNEL_H

#include "QtGui/QColor"
#include "QtGui/QMatrix"

class QSettings;

#include "GUI/Subscriber.h"
#include "Messages/Video.h"

#include "History.h"
#include "PeakBarCollection.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Messages {
class Header;
class MetaTypeInfo;
} // namespace Messages
namespace GUI {

class ServiceEntry;

namespace AScope {

/** Manages a Subscriber connection to a remote data publisher for Messages::Video data. Each instance has a
    unique History slot value which it uses to post incoming data to the application's History object. Instances
    also have a user-configurable color value which is used to distinguish its data from that of other channels.
*/
class VideoChannel : public QObject {
    Q_OBJECT
public:
    /** Obtain the log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor. Used when creating a new channel

        \param history object that records incoming data

        \param name name of the subscription channel
    */
    VideoChannel(History& history, const QString& name);

    /** Destructor. Releases any allocated history slot.
     */
    ~VideoChannel();

    /** Obtain the name of the subscription channel.

        \return name
    */
    const QString& getName() const { return name_; }

    /** Obtain the color used to draw data from the channel

        \return QColor reference
    */
    const QColor& getColor() const { return color_; }

    /** Obtain the history slot assigned to the channel.

        \return
    */
    size_t getHistorySlot() const { return historySlot_; }

    /** Obtain the number of times the channel is shown in a Visualizer view.

        \return
    */
    int getDisplayCount() const { return displayCount_; }

    const PeakBarCollection& getPeakBars() const { return peakBars_; }

    /** Determine if there are active ChannelConnection objects containing this channel.

        \return true if so
    */
    bool isDisplayed() const { return displayCount_ > 0; }

    /** Channel is associated with a new ChannelConnection object. Causes the channel to connect to the data
        publisher if not already connected.

        \return history slot assigned to the channel
    */
    size_t displayAdded();

    /** Channel is no longer associated with a ChannelConnection object. If there are no more ChannelConnection
        objects, then drop the connection to the data publisher.
    */
    void displayDropped();

    /** Obtain the connection state of the channel.

        \return true if connected, false otherwise
    */
    bool isConnected() const;

    /** Provide the connection information to use to establish a connection to a data publisher. If a connection
        currently exists, drop it and start anew.

        \param serviceEntry connection information
    */
    void useServiceEntry(ServiceEntry* serviceEntry);

    /** Change the color used when drawing the data from the channel.

        \param color new color to use
    */
    void setColor(const QColor& color);

    void setSampleToVoltageScaling(int sampleMin, int sampleMax, double voltageMin, double voltageMax);

    double getVoltageForSample(int sample) const { return (sample - sampleMin_) * voltageScale_ + voltageMin_; }

    void restoreFromSettings(QSettings& settings);

    void saveToSettings(QSettings& settings) const;

    int getSampleMin() const { return sampleMin_; }

    int getSampleMax() const { return sampleMax_; }

    double getVoltageMin() const { return voltageMin_; }

    double getVoltageMax() const { return voltageMax_; }

public slots:

    /** Notification from the application that it is shutting down. Stops the reader thread.
     */
    void shutdown();

signals:

    /** Notification sent out when the connection to a publisher has been established.
     */
    void connected();

    /** Notification sent out when the connection to a publisher has been broken.
     */
    void disconnected();

    /** Notification sent out when the channel color has changed.
     */
    void colorChanged(const QColor& color);

    /** Notification sent out when the sample-to-voltage scaling has changed.
     */
    void sampleToVoltageScalingChanged();

protected slots:

    /** Notification from the Subscriber object of incoming data.

        \param data incoming message
    */
    void readerIncoming();

private:
    const Messages::MetaTypeInfo& videoInfo_;
    History& history_;
    PeakBarCollection peakBars_;
    size_t historySlot_;
    Subscriber* reader_;
    QString name_;
    QColor color_;
    int sampleMin_;
    int sampleMax_;
    double voltageMin_;
    double voltageMax_;
    double voltageScale_;
    int displayCount_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
