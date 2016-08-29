#ifndef SIDECAR_GUI_BSCOPE_HISTORYSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_HISTORYSETTINGS_H

#include "GUI/BoolSetting.h"
#include "GUI/IntSetting.h"
#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {
namespace BScope {

class HistorySettings : public SettingsBlock
{
    Q_OBJECT
    using Super = SettingsBlock;
public:

    HistorySettings(IntSetting* frameCount,
                    BoolSetting* frameHasGrid,
                    BoolSetting* frameHasRangeMap,
                    BoolSetting* frameHasExtractions,
                    BoolSetting* frameHasRangeTruths,
                    BoolSetting* frameHasBugPlots);

    int getFrameCount() const { return frameCount_->getValue(); }

    bool getFrameHasGrid() const { return frameHasGrid_->getValue(); }

    bool getFrameHasRangeMap() const { return frameHasRangeMap_->getValue(); }

    bool getFrameHasExtractions() const
	{ return frameHasExtractions_->getValue(); }

    bool getFrameHasRangeTruths() const
	{ return frameHasRangeTruths_->getValue(); }

    bool getFrameHasBugPlots() const { return frameHasBugPlots_->getValue(); }

signals:

    void frameCountChanged(int size);

private:

    IntSetting* frameCount_;
    BoolSetting* frameHasGrid_;
    BoolSetting* frameHasRangeMap_;
    BoolSetting* frameHasExtractions_;
    BoolSetting* frameHasRangeTruths_;
    BoolSetting* frameHasBugPlots_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
