#include "CollectionItem.h"
#include "App.h"
#include "ServicesModel.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

void
CollectionItem::childAdded(TreeViewItem* child)
{
    accumulateChildStats(child);
}

void
CollectionItem::childRemoved(TreeViewItem* child)
{
    recalculateStats();
}

void
CollectionItem::updateCollectionStats(CollectionStats& stats) const
{
    stats += stats_;
}

QVariant
CollectionItem::getNameDataValue(int role) const
{
    if (role == Qt::ForegroundRole && isExpanded()) return GetTextColor();
    return Super::getNameDataValue(role);
}

QVariant
CollectionItem::getStateDataValue(int role) const
{
    if (isExpanded()) return QVariant();
    if (role != Qt::DisplayRole) return Super::getStateDataValue(role);
    return IO::ProcessingState::GetName(getProcessingState());
}

QVariant
CollectionItem::getRecordingDataValue(int role) const
{
    if (isExpanded()) return QVariant();

    if (role != Qt::DisplayRole) return GetRecordingDataValue(false, role);

    int will = stats_.getWillRecordCount();
    if (!will) return QVariant();

    if (App::GetApp()->isRecording()) {
        int current = stats_.getIsRecordingCount();
        if (current != will) return QString("%1 of %2").arg(current).arg(will);
        return "REC";
    }

    int can = stats_.getCanRecordCount();
    if (will != can) return QString("%1 of %2").arg(will).arg(can);
    return "ALL";
}

QVariant
CollectionItem::getPendingCountValue(int role) const
{
    if (isExpanded()) return QVariant();
    if (role != Qt::DisplayRole) return Super::getPendingCountValue(role);
    if (!isRecording()) return int(stats_.getPendingQueueCount());
    return QString("%1 (%2)").arg(stats_.getPendingQueueCount()).arg(stats_.getRecordingQueueCount());
}

QVariant
CollectionItem::getRateDataValue(int role) const
{
    if (isExpanded()) return QVariant();

    int dropCount = stats_.getDropCount();
    int dupeCount = stats_.getDupeCount();

    if (role == Qt::ForegroundRole) {
        if (dropCount || dupeCount) return GetFailureColor();

        switch (getActiveState()) {
        case kIdle: return GetFailureColor();

        case kPartial:
        case kNotUsingData: return GetWarningColor();

        case kActive:
        default: return GetOKColor();
        }
    }

    if (role != Qt::DisplayRole) return Super::getRateDataValue(role);

    QString status = QString("%1 of %2 active").arg(stats_.getActiveCount()).arg(stats_.getLeafCount());

    if (dropCount) status += QString(" Drops: %3").arg(dropCount);
    if (dupeCount) status += QString(" Dupes: %3").arg(dupeCount);

    return status;
}

QVariant
CollectionItem::getErrorDataValue(int role) const
{
    if (isExpanded()) return QVariant();
    if (role != Qt::DisplayRole) return Super::getErrorDataValue(role);
    return stats_.getError();
}

void
CollectionItem::beforeUpdate()
{
    stats_.clear();
}

void
CollectionItem::afterUpdate()
{
    updateChildren();

    for (int index = 0; index < getNumChildren(); ++index) accumulateChildStats(getChild(index));

    setOK(stats_.isOK());
    setProcessingState(stats_.getProcessingState());

    size_t leafCount = stats_.getLeafCount();
    if (stats_.getNotUsingDataCount() == leafCount)
        setActiveState(kNotUsingData);
    else if (stats_.getIdleCount() == leafCount)
        setActiveState(kIdle);
    else if (stats_.getActiveCount() == leafCount)
        setActiveState(kActive);
    else
        setActiveState(kPartial);
}

void
CollectionItem::accumulateChildStats(TreeViewItem* child)
{
    child->updateCollectionStats(stats_);
}

void
CollectionItem::recalculateStats()
{
    stats_.clear();
    for (int index = 0; index < getNumChildren(); ++index) accumulateChildStats(getChild(index));
}
