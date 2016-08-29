#ifndef SIDECAR_GUI_HISTORYBUFFER_H // -*- C++ -*-
#define SIDECAR_GUI_HISTORYBUFFER_H

#include "QtCore/QList"

#include "HistoryFrame.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

/** Container of HistoryFrame objects that represent snapshots of past data. The size of the container is
    bounded by a duration parameter, which limits how old the oldest record in the buffer may be. Does not
    delete old frames, but rather places them on a free list. For near-constant data rates, this should allow
    the HistoryBuffer to reach a stable point where it no longer allocates new frames.
*/
class HistoryBuffer
{
public:

    static Logger::Log& Log();

    /** Constructor.

        \param duration how old in seconds the oldest record may be before it
        is removed.
    */
    HistoryBuffer(int duration);

    /** Change the duration value

        \param duration new duration value in seconds
    */
    void setDuration(int duration);

    /** Add a new HistoryFrame to the buffer, and prune any old entries. Note: makes a copy of the given frame.

        \param current reference to new frame to add
    */
    void record(int bufferIndex, const Messages::PRIMessage::Ref& msg);

    /** Obtain the number of frames held in the buffer. Note that since buffer size is determined by time, this
        value might not the same from call to call.

        \return buffer size
    */
    int size() const { return size_; }

    /** Fetch the frame from the buffer at a given position.

        \param index position to fetch from 

        \return found frame
    */
    const HistoryFrame& getFrame(int index) const;

    void addMessageBuffer();

    void removeMessageBuffer(int index);

    void clear();

private:
    using MessageBuffer = QList<Messages::PRIMessage::Ref>;
    using MessageBufferList = QList<MessageBuffer*>;
    MessageBufferList buffers_;
    HistoryFrame* frame_;
    int duration_;
    int size_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
