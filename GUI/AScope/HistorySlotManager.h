#ifndef SIDECAR_GUI_HISTORYSLOTMANAGER_H // -*- C++ -*-
#define SIDECAR_GUI_HISTORYSLOTMANAGER_H

#include <vector>

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

class HistoryBuffer;

/** Assigns indices in a HistoryFrame to slot values obtained from History::allocateSlot(). When a Visualizer
    adds a new data connection, it obtains a new slot from the History manager, but until data arrives over the
    connection, it does not have an assignment to a location in a HistoryFrame. This indirection was done to
    keep HistoryFrame objects as small as possible. Once the channel receives its first message, and attempts to
    store into a HistoryFrame, the HistorySlotManager will allocate an index for it to use to write into the
    HistoryFrame. When a channel connection goes away, the index it used is removed, and the set of active
    indices in the allocated_ member is adjusted so that there are no gaps in future HistoryFrame objects.
*/
class HistorySlotManager 
{
public:

    static Logger::Log& Log();

    /** Constructor.
     */
    HistorySlotManager(HistoryBuffer& buffer)
	: buffer_(buffer), active_(0), allocated_() {}

    size_t getNumAllocated() const { return allocated_.size(); }

    /** Allocate a new slot value for a channel connection.

        \return allocated slot
    */
    size_t allocate();

    /** Release a previously allocated slot

        \param slot value to release.
    */
    void release(size_t slot);

    /** Obtain the current index for a slot or allocate a new index

        \param slot value to work with

        \return assigned HistoryFrame index value
    */
    int allocateIndexForSlot(size_t slot);

    /** Obtain the index allocated for a slot. NOTE: allocateIndexForSlot() must have been previously called
        with the given slot value.

        \param slot value to work with

        \return assigned HistoryFrame index value
    */
    int getIndexForSlot(size_t slot) const
	{ return allocated_[slot]; }

private:
    HistoryBuffer& buffer_;
    int active_;
    std::vector<int> allocated_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
