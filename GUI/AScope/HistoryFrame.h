#ifndef SIDECAR_GUI_HISTORYFRAME_H // -*- C++ -*-
#define SIDECAR_GUI_HISTORYFRAME_H

#include "QtCore/QList"

#include "Messages/PRIMessage.h"
#include "Time/TimeStamp.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace AScope {

/** Collection of Messages::PRIMessage objects that represent a viewable entity in the AScope. The order of the
    messages in an entry are based on the assignments done by a HistorySlotManager object.
*/
class HistoryFrame {
public:
    /** Log device used by HistoryFrame objects

        \return log device
    */
    static Logger::Log& Log();

    HistoryFrame() : messages_(), timeStamp_(), lastValid_(-1) {}

    /** Erase the references to all held messages.
     */
    void clear();

    /** Add an empty reference to the end of the frame.
     */
    void expand();

    /** Remove the last message slot from the frame, making it smaller.
     */
    void shrink();

    /** Append a message to the end of the frame.

        \param msg the message to add.
    */
    void append(const Messages::PRIMessage::Ref& msg);

    /** Overwrite an existing entry with a new message reference. NOTE: unlike getMessage(), the index value
        given to this method must be valid, or else QList will throw an exception.

        \param index position to update

        \param message new value to hold (may be null)
    */
    void update(int index, const Messages::PRIMessage::Ref& msg);

    /** Overwrite and existing entry with a null message reference. The given index value must be valid, or else
        QList will throw an exception.

        \param index position to clear
    */
    void clearMessage(int index);

    /** Determine if there is a valid message at a given index. Safe to call with invalid index values.

        \param index value to check

        \return true if so
    */
    bool hasMessage(int index) const { return getMessage(index).get() != nullptr; }

    /** Obtain a message at a given index. NOTE: the QList::value() method will always return a value, event if
        index is invalid, in which case the returned message reference may refer to no message.

        \param index which message to fetch

        \return message reference
    */
    Messages::PRIMessage::Ref getMessage(int index) const { return messages_.value(index); }

    /** Fetch the first non-NULL message in the collection. This is usually the first entry in the list, but
        that is not guaranteed.

        \return
    */
    Messages::PRIMessage::Ref getSomeMessage() const { return getMessage(lastValid_); }

    /** Determine if the frame has no messages.

        \return true if so
    */
    bool empty() const { return lastValid_ == -1; }

    /** Obtain the timestamp of the last message added to the collection.

        \return
    */
    double getTimeStamp() const { return timeStamp_; }

private:
    void updateLastValid();

    /** Set the entry's timestamp based on values from a message.

        \param message source of the timestamp
    */
    void setTimeStamp(const Messages::PRIMessage::Ref& message);

    QList<Messages::PRIMessage::Ref> messages_;
    double timeStamp_;
    int lastValid_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
