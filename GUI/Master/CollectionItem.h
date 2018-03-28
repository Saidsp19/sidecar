#ifndef SIDECAR_GUI_COLLECTIONITEM_H // -*- C++ -*-
#define SIDECAR_GUI_COLLECTIONITEM_H

#include "CollectionStats.h"
#include "TreeViewItem.h"

namespace SideCar {
namespace GUI {
namespace Master {

/** Specialization of TreeViewItem that represents a collection of TreeViewItem objects. Defines and collects
    statistics for the TreeViewItem objects in its collection.
*/
class CollectionItem : public TreeViewItem {
    Q_OBJECT
    using Super = TreeViewItem;

public:
    void updateCollectionStats(CollectionStats& stats) const;

    bool canExpand() const { return true; }

    QVariant getNameDataValue(int role) const;

    QVariant getStateDataValue(int role) const;

    QVariant getRecordingDataValue(int role) const;

    QVariant getPendingCountValue(int role) const;

    QVariant getRateDataValue(int role) const;

    QVariant getErrorDataValue(int role) const;

    IO::ProcessingState::Value getProcessingState() const { return stats_.getProcessingState(); }

    const CollectionStats& getCollectionStats() const { return stats_; }

    bool canRecord() const { return stats_.getCanRecordCount() > 0; }

    bool willRecord() const { return stats_.getWillRecordCount() > 0; }

    bool isRecording() const { return stats_.getIsRecordingCount() > 0; }

    void recalculateStats();

protected:
    CollectionItem() : Super(), stats_() {}

    CollectionItem(const IO::StatusBase& status, TreeViewItem* parent) : Super(status, parent), stats_() {}

    virtual void updateChildren() = 0;

    void beforeUpdate();

    void afterUpdate();

    void accumulateChildStats(TreeViewItem* child);

private:
    void childAdded(TreeViewItem* child);

    void childRemoved(TreeViewItem* child);

    CollectionStats stats_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
