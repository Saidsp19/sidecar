#include <cmath>

#include "GUI/LogUtils.h"
#include "GUI/MessageList.h"
#include "Utils/Utils.h"

#include "History.h"

using namespace Utils;
using namespace SideCar;
using namespace SideCar::GUI::AScope;

enum SlotMappingTokens {
    kSlotUnallocated = -2,
    kSlotNeedsIndex = -1
};

Logger::Log&
History::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.History");
    return log_;
}

History::History(QObject* parent)
    : QObject(parent), liveFrame_(), buffers_(), frameCount_(0),
      slotMapping_(), duration_(30), enabled_(false), pastFrozen_(false)
{
    ;
}

int
History::allocateSlot()
{
    Logger::ProcLog log("allocateSlot", Log());
    LOGINFO << "existing: " << slotMapping_.size() << std::endl;

    // Obtain a unique slot index for a data channel. The slot index stays constant durng the lifetime of the
    // data channel. First, try looking for an available entry in the slot map.
    //
    for (int index = 0; index < slotMapping_.size(); ++index) {
	if (slotMapping_[index] == kSlotUnallocated) {
	    slotMapping_[index] = kSlotNeedsIndex;
	    LOGDEBUG << "allocated " << index << std::endl;
	    return index;
	}
    }

    // No available entry found above.
    //
    slotMapping_.push_back(kSlotNeedsIndex);
    LOGDEBUG << "allocated " << (slotMapping_.size() - 1) << std::endl;
    return slotMapping_.size() - 1;
}

void
History::releaseSlot(int slot)
{
    static Logger::ProcLog log("releaseSlot", Log());
    LOGINFO << "slot: " << slot << " buffers.size: " << buffers_.size()
	    << " slotMapping.size: " << slotMapping_.size() << std::endl;

    // If the slot was assigned a buffer for past data, we need to remove the buffer and possibly update the
    // slot map.
    //
    int oldIndex = slotMapping_[slot];
    if (oldIndex >= 0) {

	// Forget the history buffer assigned to the slot and recalculate the overall size.
	//
	delete buffers_.takeAt(oldIndex);
	frameCount_ = 0;
	foreach (MessageBuffer* buffer, buffers_) {
	    if (buffer->size() > frameCount_)
		frameCount_ = buffer->size();
	}

	// Need to adjust entries in the slot map with indices smaller than the one we just removed.
	//
	for (int index = 0; index < slotMapping_.size(); ++index) {
	    if (slotMapping_[index] >= oldIndex)
		slotMapping_[index] -= 1;
	}

	// We were using an entry in the live frame, but not any more.
	//
	liveFrame_.shrink();
    }

    // Remove the index from the slot mapping. If removing the last entry in the map, try collapsing the vector
    // by removing other entries with kSlotUnallocated values.
    //
    if (slot == slotMapping_.size() - 1) {
	do {
	    slotMapping_.pop_back();
	}
	while (! slotMapping_.empty() &&
               slotMapping_.back() == kSlotUnallocated);
    }
    else {
	slotMapping_[slot] = kSlotUnallocated;
    }
}

void
History::update(int slot, const MessageList& data)
{
    static Logger::ProcLog log("update", Log());
    LOGINFO << "slot: " << slot << " count: " << data.size() << std::endl;

    // Obtain an index into the frames from the slot value assigned by allocateSlot(). Since this value may
    // change depending on future releaseSlot() calls, it should not be held outside of this routine.
    //
    int bufferIndex = slotMapping_[slot];
    if (bufferIndex == kSlotNeedsIndex) {
	bufferIndex = buffers_.size();
	slotMapping_[slot] = bufferIndex;
	buffers_.append(new MessageBuffer);
	liveFrame_.expand();
    }

    LOGDEBUG << "bufferIndex: " << bufferIndex << std::endl;

    Messages::PRIMessage::Ref msg =
	boost::dynamic_pointer_cast<Messages::PRIMessage>(data.back());

    liveFrame_.update(bufferIndex, msg);
    emit liveFrameChanged();

    if (! enabled_)
	return;

    if (pastFrozen_)
	return;

    // Prune old messages from all of the message buffers. Take out messages that are older than the time of the
    // last message - duration.
    //
    frameCount_ = 0;
    Time::TimeStamp limit = msg->getEmittedTimeStamp();
    limit -= duration_;
    MessageBuffer* buffer;
    foreach (buffer, buffers_) {

	// The beginning of the buffer contains the oldest message. Locate the first message from the start that
	// is wanted.
	//
	MessageBuffer::iterator pos = buffer->begin();
	while (pos != buffer->end() &&
               (*pos)->getEmittedTimeStamp() < limit) {
	    pos->reset();
	    ++pos;
	}

	// Remove them.
	//
	if (pos != buffer->begin()) {
	    buffer->erase(buffer->begin(), pos);
	}

	// Keep track of the largest buffer size.
	//
	if (frameCount_ < buffer->size())
	    frameCount_ = buffer->size();
    }

    // Append the incoming messages to the proper buffer.
    //
    buffer = buffers_[bufferIndex];
    for (size_t index = 0; index < data.size(); ++index) {
	msg = boost::dynamic_pointer_cast<Messages::PRIMessage>(
	    data[index]);
	buffer->append(msg);
    }

    if (frameCount_ < buffer->size())
	frameCount_ = buffer->size();
}

void
History::setEnabled(bool enabled)
{
    static Logger::ProcLog log("setEnabled", Log());
    LOGINFO << enabled << std::endl;
    enabled_ = enabled;
    if (! enabled) {
	foreach (MessageBuffer* buffer, buffers_) {
	    buffer->clear();
	}
    }
}

void
History::freezePast(bool state)
{
    if (pastFrozen_  != state) {
	pastFrozen_ = state;
	if (state)
	    emit pastFrozen();
	else
	    emit pastThawed();
    }
}

void
History::setDuration(int duration)
{
    static Logger::ProcLog log("setDuration", Log());
    LOGINFO << duration << std::endl;
    if (duration_ != duration) {
	duration_ = duration;
    }
}

HistoryFrame
History::getPastFrame(int position) const
{
    static Logger::ProcLog log("getFrame", Log());
    LOGINFO << "position: " << position << std::endl;

    HistoryFrame frame;
    int index = frameCount_ - position - 1;

    foreach (MessageBuffer* buffer, buffers_) {

	// Using QList::value() to always get a value, even for an invalid position.
	//
	frame.append(buffer->value(index));
    }

    return frame;
}

Messages::PRIMessage::Ref
History::getLiveMessage(int slot) const
{
    static Logger::ProcLog log("getMessage", Log());
    LOGINFO << "slot: " << slot << std::endl;

    // Using QList::value() to always return a value, even for invalid indices.
    //
    return liveFrame_.getMessage(slotMapping_[slot]);
}

Messages::PRIMessage::Ref
History::getPastMessage(int position, int slot) const
{
    static Logger::ProcLog log("getMessage", Log());
    return getPastFrame(position).getMessage(slotMapping_[slot]);
}
