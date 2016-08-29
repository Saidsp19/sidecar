#ifndef SIDECAR_GUI_ASCOPE_CHANNELCONNECTION_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_CHANNELCONNECTION_H

#include "QtCore/QList"
#include "QtCore/QObject"
#include "QtCore/QPointF"
#include "QtGui/QColor"

#include "PeakBarRenderer.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

class VideoChannel;
class Visualizer;

/** Instances represent a link between an AScope Visualizer and a VideoChannel. More than one Visualizer may
    have a connection to the same VideoChannel object. A channel connection may be frozen so that future updates
    from the video channel have no effect on the Visualizer.
*/
class ChannelConnection : public QObject
{
    Q_OBJECT
public:

    /** Obtain the log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor. Connect signal/slots between two Plotter and VideoChannel objects.

        \param visualizer Visualizer object to connect

        \param channel VideoChannel object to connect
    */
    ChannelConnection(Visualizer& visualizer, VideoChannel& channel,
                      bool visible, bool showPeakBars);

    /** Destructor. Drops the link with the VideoChannel.
     */
    ~ChannelConnection();

    /** Obtain the video channel associated with the connection.

        \return VideoChannel reference
    */
    VideoChannel& getChannel() const { return channel_; }

    /** Obtain the visualizer associated with the connection. This is the object that is responsible for
        rendering the channel data.

        \return Visualizer reference
    */
    Visualizer& getVisualizer() const { return visualizer_; }

    /** Obtain the visibility state of the channel.

        \return true if visible
    */
    bool isVisible() const { return visible_; }

    /** Determine if peak bars are shown for this channel connection. Combines the channel-specific setting with
        the setting from the global PeakBarSettings object.

        \return 
    */
    bool isShowingPeakBars() const { return showPeakBars_; }

    bool isReallyShowingPeakBars() const;

    /** Obtain the frozen state of the channel.

        \return true if frozen
    */
    bool isFrozen() const { return frozen_; }

    bool isReallyFrozen() const;

    const QColor& getColor() const { return color_; }

    int getHistorySlot() const { return historySlot_; }

signals:

    void redisplay();

public slots:

    /** Change the visibility state of the connetion

        \param state new visibility state
    */
    void setVisible(bool state);

    void setShowPeakBars(bool state);

    /** Change the frozen state of the connetion

        \param state new frozen state
    */
    void setFrozen(bool state);

    void setColor(const QColor& color);

private slots:

    /** Notification from Visualizer that its view transformation has changed. We notify our PeakBarRenderer and
	zap the last plotPoints_ contents so that they will get regenerated at the next
	Visualizer::paintEvent().
    */
    void clearCache();

private:
    VideoChannel& channel_;
    Visualizer& visualizer_;
    PeakBarRenderer peakBarRenderer_;
    Messages::PRIMessage::Ref lastRendered_;
    std::vector<QPointF> plotPoints_;

    size_t historySlot_;
    QColor color_;
    bool visible_;
    bool showPeakBars_;
    bool frozen_;

    friend class Visualizer;
};

/** Container class for ChannelConnection objects. QList is similar to std::deque (fast insertions at both ends
    of the container) but apparently faster and smaller for pointer objects.
*/
using ChannelConnectionList = QList<ChannelConnection*>;

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
