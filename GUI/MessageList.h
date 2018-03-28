#ifndef SIDECAR_GUI_MESSAGELIST_H // -*- C++ -*-
#define SIDECAR_GUI_MESSAGELIST_H

#include <vector>

#include "Messages/Header.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

/** Collection of SideCar::Messages::Header objects received from a network connection. Defines a Qt meta type
    for the class so that shared references to instances of the class may be contained in signals sent across
    threads. A specific instance of this may be found in GUI::ReaderThread which manages a separate thread for
    reading UDP/TCP data, storing the whole messages in instances of ACE_Message_Block. The mesage blocks are
    appended to a MessageBlockList, a reference to which is then used in its incoming() signal.

    Note that since this object may be shared among threads, it should be
    treated as a read-only collection once it has been emitted in signal. Also,
    the MessageBlockList will properly destroy the contained ACE_Message_Block
    objects when the MessageList instance is destroyed. Therefore, one should
    always use the ACE_Message_Block::duplicate() method to obtain a personal
    copy of a held mesage block, for instance when giving it to an
    IO::MessageManager instance.
*/
class MessageList {
public:
    using Container = std::vector<Messages::Header::Ref>;
    using const_iterator = Container::const_iterator;

    /** Constructor. Create an emtpy list of ACE_Message_Block objects.
     */
    MessageList() : messages_() { messages_.reserve(32); }

    bool empty() const { return messages_.empty(); }

    size_t size() const { return messages_.size(); }

    void clear() { messages_.clear(); }

    void push_back(const Messages::Header::Ref& msg) { messages_.push_back(msg); }

    Messages::Header::Ref front() const { return messages_.front(); }

    Messages::Header::Ref back() const { return messages_.back(); }

    Messages::Header::Ref operator[](size_t index) const { return messages_[index]; }

    const_iterator begin() const { return messages_.begin(); }

    const_iterator end() const { return messages_.end(); }

private:
    Container messages_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
