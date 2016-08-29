#ifndef SIDECAR_GUI_PPIDISPLAY_HISTORYSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_HISTORYSETTINGS_H

#include "GUI/IntSetting.h"
#include "GUI/OnOffSettingsBlock.h"

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class HistorySettings : public OnOffSettingsBlock
{
    Q_OBJECT
    using Super = OnOffSettingsBlock;
public:

    HistorySettings(BoolSetting* enabled, IntSetting* retentionSize);

    int getRetentionSize() const { return retentionSize_->getValue(); }

signals:

    void retentionSizeChanged(int size);

private:
    IntSetting* retentionSize_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
