#ifndef SIDECAR_GUI_RENDERINFO_H // -*- C++ -*-
#define SIDECAR_GUI_RENDERINFO_H

#include <vector>

#include "QtCore/QLineF"
#include "QtCore/QObject"
#include "QtCore/QPointF"
#include "QtCore/QSizeF"
#include "QtGui/QColor"

#include "Messages/PRIMessage.h"

namespace SideCar {
namespace GUI {
namespace AScope {

class PeakBarCollection;
class VideoChannel;
class Visualizer;

/** Collection of render attributes associated with a particular ChannelConnection object. Contains state and
    cached data to improve rendering times when data from a channel does not change.

    A Visualizer widget may subscribe to one or more input channels. To keep
    rendering as fast as possible, input data is first converted into render
    data, and then at the next Visualizer update, it draws the render data of
    all of the input channels in one shot.
*/
class RenderInfo : public QObject {
    Q_OBJECT
public:
    using QPointFVector = std::vector<QPointF>;

    /** Constructor

        \param visualizer the Visualizer that uses data in this instance

        \param color the color to use when rendering data

        \param visible state of visibility for the channel
    */
    RenderInfo(Visualizer& visualizer, PeakBarCollection& peakBars, const QColor& color, bool visible,
               bool showPeakBars);

    /** Assign a unique slot from the History manager for this instance.

        \param historySlot value to assign
    */
    void setHistorySlot(size_t historySlot) { historySlot_ = historySlot; }

    /** Obtain whether channel data is visibile.

        \return true if so
    */
    bool isVisible() const { return visible_; }

    /** Obtain whether channel data is frozen at some frame.

        \return true if so
    */
    bool isFrozen() const { return frozen_; }

    /**

        \return
    */
    bool showPeakBars() const { return showPeakBars_; }

public slots:

    /** Change the rendering color for the channel

        \param color new color to use
    */
    void setColor(const QColor& color);

    /** Change the visibility state of the channel

        \param state new state
    */
    void setVisible(bool state);

    /** Change the frozen state of the channel. If frozen, future data messages will not change the rendering of
        a channel.

        \param state new state
    */
    void setFrozen(bool state);

    /**

        \param state
    */
    void setShowPeakBars(bool state);

private slots:

    void adjustPeakBars();

    void updatePeakBarCache();

    void visualizerTransformChanged();

private:
    Visualizer& visualizer_;
    PeakBarCollection& peakBars_;

    size_t historySlot_;
    QColor color_;
    bool visible_;
    bool showPeakBars_;
    bool frozen_;

    Messages::PRIMessage::Ref lastRendered_;
    QPointFVector plotPoints_;

    QSizeF barSize_;
    QPointF origin_;
    std::vector<PeakBarValue> peakBarValues_;

    friend class Visualizer;
    friend class PeakBarCollection;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
