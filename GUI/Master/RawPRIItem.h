#ifndef SIDECAR_GUI_MASTER_RAWPRIITEM_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_RAWPRIITEM_H

#include "ControllerItem.h"

namespace SideCar {
namespace GUI {
namespace Master {

class RawPRIItem : public ControllerItem
{
    Q_OBJECT
    using Super = ControllerItem;
public:

    RawPRIItem(const Algorithms::ControllerStatus& status, StreamItem* parent)
	: Super(status, parent), dupeCount_(0), dropCount_(0),
	  recordingDupeCount_(0), recordingDropCount_(0) {}

    QVariant getInfoDataValue(int role) const;

    void fillCollectionStats(CollectionStats& stats) const;

private:

    void afterUpdate();

    int dupeCount_;
    int dropCount_;
    int recordingDupeCount_;
    int recordingDropCount_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
