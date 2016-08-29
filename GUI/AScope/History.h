#ifndef SIDECAR_GUI_ASCOPE_HISTORY_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_HISTORY_H

#include "boost/shared_ptr.hpp"

#include "QtCore/QList"
#include "QtCore/QMetaType"
#include "QtCore/QObject"

#include "Messages/Video.h"

#include "HistoryFrame.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class MessageList;

namespace AScope {

/** Manager of a collection of HistoryFrame objects that represent unique snapshots of data from one or more
    input channels.
*/
class History : public QObject
{
    Q_OBJECT
public:

    static Logger::Log& Log();

    /** Constructor.

        \param durations number of seconds to retain

        \param enabled whether retention is enabled
    */
    History(QObject* parent);

    /** Obtain a unique slot to use for working with the history buffer.

        \return slot value
    */
    int allocateSlot();

    /** Obtain the internal mapping from a slot value to frame index. NOTE: this value may change after future
        calls to releaseSlot(), so it should not be saved. Also, the returned value may be -1 if the data
        channel assigned to the slot has not yet placed any data into its history buffer.

        \param slot value from a prior allocateSlot() call to fetch

        \return assigned index value for the slot
    */
    int getSlotIndex(int slot) const { return slotMapping_[slot]; }

    /** Remove an allocated slot from the history buffer. Future calls to update() or getMessage() with the
        given slot value will return a NULL reference.

        \param slot index to remove
    */
    void releaseSlot(int slot);

    /** Update the historical record with a one or more data messages.

        \param slot unique ID of the data provider, obtained by an earlier call
        to allocateSlot().

        \param data collection of raw incoming messages to record

	\param metaType description of the incoming data
    */
    void update(int slot, const MessageList& data);

    /** Determine if retention is enabled.

        \return true if so
    */
    bool isEnabled() const { return enabled_; }

    /** Obtain the number of seconds retained.

        \return retention count
    */
    int getDuration() const { return duration_; }

    /** Obtain the number of frames in the historical past. Since pruning of the records is done by time, this
        value is not a constant.

        \return past size
    */
    int getFrameCount() const { return frameCount_; }

    /** Obtain the current frame of data.

        \return HistoryFrame pointer
    */
    const HistoryFrame& getLiveFrame() const { return liveFrame_; }

    /** Obtain a frame of data from sometime in the past if position is non-zero, or else the current frame.

        \param position position in a HistoryBuffer of the data to fetch

        \return HistoryFrame pointer
    */
    HistoryFrame getPastFrame(int position) const;

    /** Obtain the live data of a slot.

        \param position which entry to obtain

        \param slot which message of an entry to obtain

        \return a PRIMessage reference (may be a NULL reference)
    */
    Messages::PRIMessage::Ref getLiveMessage(int slot) const;

    /** Obtain the frozen data of a slot.

        \param position which entry to obtain

        \param slot which message of an entry to obtain

        \return a PRIMessage reference (may be a NULL reference)
    */
    Messages::PRIMessage::Ref getPastMessage(int position, int slot)
	const;

signals:

    /** Notification sent out when a new live frame is available.

        \param frame 
    */
    void liveFrameChanged();

    void pastFrozen();

    void pastThawed();

public slots:

    /** Change whether retention is enabled

        \param enabled new retention state
    */
    void setEnabled(bool enabled);

    /** Freeze the past buffer so that it may be viewed, or thaw it to accumulate new live data. Emits the
	pastFrozen() or pastThawed() signal depending on which state the past buffer is in. signal.
    */
    void freezePast(bool state);

    /** Change the number of seconds retained.

        \param duration seconds retained
    */
    void setDuration(int duration);

private:
    HistoryFrame liveFrame_;
    using MessageBuffer = QList<Messages::PRIMessage::Ref>;
    using MessageBufferList = QList<MessageBuffer*>;
    MessageBufferList buffers_;
    int frameCount_;
    QList<int> slotMapping_;
    int duration_;
    bool enabled_;
    bool pastFrozen_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
