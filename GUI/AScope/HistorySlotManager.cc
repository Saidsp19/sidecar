#include "GUI/LogUtils.h"

#include "HistoryBuffer.h"
#include "HistorySlotManager.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::AScope;

Logger::Log&
HistorySlotManager::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.HistorySlotManager");
    return log_;
}

size_t
HistorySlotManager::allocate()
{
    static Logger::ProcLog log("allocate", Log());
    LOGINFO << "active: " << active_ << " allocated: " << allocated_.size()
	    << std::endl;
    allocated_.push_back(-1);
    return allocated_.size() - 1;
}

void
HistorySlotManager::release(size_t slot)
{
    static Logger::ProcLog log("release", Log());
    LOGINFO << "slot: " << slot << " active: " << active_ << " allocated: "
	    << allocated_.size() << std::endl;

    int oldIndex = allocated_[slot];
    if (oldIndex != -1) {

	buffer_.removeMessageBuffer(oldIndex);

	if (slot == allocated_.size() - 1)
	    allocated_.pop_back();
	else
	    allocated_[slot] = -1;

	for (size_t slot = 0; slot < allocated_.size(); ++slot) {
	    if (allocated_[slot] >= oldIndex)
		allocated_[slot] -= 1;
	}

	--active_;
    }

    LOGDEBUG << "active: " << active_ << " allocated: " << allocated_.size()
	     << std::endl;
}

int
HistorySlotManager::allocateIndexForSlot(size_t slot)
{
    int index = allocated_[slot];
    if (index == -1) {
	index = active_++;
	allocated_[slot] = index;
	buffer_.addMessageBuffer();
    }

    return index;
}
