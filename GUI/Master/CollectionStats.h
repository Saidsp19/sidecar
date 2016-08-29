#ifndef SIDECAR_GUI_MASTER_COLLECTIONSTATS_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_COLLECTIONSTATS_H

#include <vector>

#include "QtCore/QString"

#include "IO/ProcessingState.h"

namespace SideCar {
namespace GUI {
namespace Master {

/** Statistics gathered for the children of a CollectionItem object.
 */
class CollectionStats
{
public:

    enum Index {

	// TaskItem stats
	//
	kLeafCount,	    ///< Number of leaf nodes in collection
	kOKCount,	    ///< Number of OK leaf nodes
	kIdleCount,	    ///< Number of idle leaf nodes
	kActiveCount,	    ///< Number of active leaf nodes
	kNotUsingDataCount, ///< Number of leaf nodes not pulling data
	kPendingQueueCount, ///< Sum of pending queue sizes of leaf nodes

	// ControllerItem stats
	//
	kCanRecordCount,	/// Number of leaf nodes that can record
	kWillRecordCount,	/// Number of leaf nodes with recording enabled
	kIsRecordingCount,	/// Number of leaf nodes that are recording
	kRecordingQueueCount,	/// Sum of recording queue sizes of leaf nodes

	// RawPRIItem stats
	//
	kDropCount,		/// Sum of running drop counts of leaf nodes
	kDupeCount,		/// Sum of running dupe counts of leaf nodes

	kNumStats
    };

    CollectionStats();

    void clear();

    CollectionStats& operator+=(const CollectionStats& rhs);

    size_t& operator[](Index index) { return counters_[index]; }
    
    size_t operator[](Index index) const { return counters_[index]; }

    size_t getLeafCount() const { return counters_[kLeafCount]; }

    size_t getCanRecordCount() const { return counters_[kCanRecordCount]; }

    size_t getWillRecordCount() const { return counters_[kWillRecordCount]; }

    size_t getIsRecordingCount() const
	{ return counters_[kIsRecordingCount]; }

    size_t getOKCount() const { return counters_[kOKCount]; }

    size_t getFailureCount() const { return getLeafCount() - getOKCount(); }

    size_t getIdleCount() const { return counters_[kIdleCount]; }

    size_t getActiveCount() const { return counters_[kActiveCount]; }

    size_t getNotUsingDataCount() const
	{ return counters_[kNotUsingDataCount]; }

    size_t getPendingQueueCount() const
	{ return counters_[kPendingQueueCount]; }

    size_t getRecordingQueueCount() const
	{ return counters_[kRecordingQueueCount]; }

    size_t getDropCount() const { return counters_[kDropCount]; }

    size_t getDupeCount() const { return counters_[kDupeCount]; }

    bool isOK() const
	{ return error_.isEmpty() && getOKCount() > 0; }

    IO::ProcessingState::Value getProcessingState() const
	{ return processingState_; }

    void setProcessingState(IO::ProcessingState::Value processingState)
	{ processingState_ = processingState; }

    const QString& getError() const { return error_; }

    void setError(const QString& error) { error_ = error; }

private:
    std::vector<size_t> counters_;
    IO::ProcessingState::Value processingState_;
    QString error_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
