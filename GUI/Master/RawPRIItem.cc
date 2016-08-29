#include "Algorithms/ControllerStatus.h"
#include "Algorithms/RawPRI/RawPRI.h"

#include "CollectionStats.h"
#include "RawPRIItem.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::GUI::Master;

QVariant
RawPRIItem::getInfoDataValue(int role) const
{
    if (dupeCount_ || dropCount_) {
	if (role == Qt::ForegroundRole)
	    return GetFailureColor();

	if (role == Qt::DisplayRole)
	    return QString("Dupes: %1 (%2)  Drops: %3 (%4)")
		.arg(dupeCount_)
		.arg(recordingDupeCount_)
		.arg(dropCount_)
		.arg(recordingDropCount_);
    }

    return Super::getInfoDataValue(role);
}

void
RawPRIItem::afterUpdate()
{
    const IO::StatusBase& status(getStatus());
    dupeCount_ = int(status[RawPRI::kDuplicates]);
    dropCount_ = int(status[RawPRI::kDrops]);
    recordingDupeCount_ = int(status[RawPRI::kRecordingDuplicates]);
    recordingDropCount_ = int(status[RawPRI::kRecordingDrops]);
    Super::afterUpdate();
}

void
RawPRIItem::fillCollectionStats(CollectionStats& stats) const
{
    Super::fillCollectionStats(stats);
    stats[CollectionStats::kDupeCount] = dupeCount_;
    stats[CollectionStats::kDropCount] = dropCount_;
    stats[CollectionStats::kRecordingDupeCount] = recordingDupeCount_;
    stats[CollectionStats::kRecordingDropCount] = recordingDropCount_;
}
