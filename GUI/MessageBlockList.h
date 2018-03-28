#ifndef SIDECAR_GUI_MESSAGEBLOCKLIST_H // -*- C++ -*-
#define SIDECAR_GUI_MESSAGEBLOCKLIST_H

#include <vector>

#include "boost/shared_ptr.hpp"

#include "QtCore/QMetaType"

namespace Logger {
class Log;
}

class ACE_Message_Block;

namespace SideCar {
namespace GUI {

/** Collection of ACE_Message_Block objects received from a network connection. Defines a Qt meta type for the
    class so that shared references to instances of the class may be contained in signals sent across threads. A
    specific instance of this may be found in GUI::ReaderThread which manages a separate thread for reading
    UDP/TCP data, storing the whole messages in instances of ACE_Message_Block. The mesage blocks are appended
    to a MessageBlockList, a reference to which is then used in its incoming() signal.

    Note that since this object may be shared among threads, it should be
    treated as a read-only collection once it has been emitted in signal. Also,
    the MessageBlockList will properly destroy the contained ACE_Message_Block
    objects when the MessageList instance is destroyed. Therefore, one should
    always use the ACE_Message_Block::duplicate() method to obtain a personal
    copy of a held mesage block, for instance when giving it to an
    IO::MessageManager instance.
*/
class MessageBlockList {
public:
    using Ref = boost::shared_ptr<MessageBlockList>;

    static Logger::Log& Log();

    static int GetAliveCount();

    /** Constructor. Create an emtpy list of ACE_Message_Block objects.
     */
    MessageBlockList();

    /** Destructor. Invoke ACE_Message_Block::release() on each held ACE_Message_Block object.
     */
    ~MessageBlockList();

    size_t size() const { return data_.size(); }

    void add(ACE_Message_Block* data) { data_.push_back(data); }

    ACE_Message_Block* getLastEntry() const;

    ACE_Message_Block* getEntry(size_t index) const;

private:
    std::vector<ACE_Message_Block*> data_;

    /** Unique Qt meta type identifier for SideCar::GUI::MessageBlockList::Ref objects. Filled in with the
        return from qRegisterMetaType() function.
    */
    static int const kMetaTypeId;
};

} // end namespace GUI
} // end namespace SideCar

/** Create a Qt meta-type declaration for MessageList::Ref objects. This lets MessageList::Ref objects act as
    (nearly) first-class citizens in the Qt type system; specifically, it allows the reference to travel inside
    of signals between threads.
*/
Q_DECLARE_METATYPE(SideCar::GUI::MessageBlockList::Ref)

/** \file
 */

#endif
