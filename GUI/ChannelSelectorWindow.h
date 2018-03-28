#ifndef SIDECAR_GUI_CHANNELSELECTORWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_CHANNELSELECTORWINDOW_H

#include "GUI/ToolWindowBase.h"

class QComboBox;

namespace SideCar {
namespace GUI {

/** Tool window that shows three QComboBox buttons that allow the user to select which Video, BinaryVideo, and
    Extractions publisher to connect to for data. Most of the work is done from inside the ChannelGroup class;
    this class simply establishes connections between the GUI and the ChannelGroup instances.
*/
class ChannelSelectorWindow : public ToolWindowBase {
    Q_OBJECT
    using Super = ToolWindowBase;

public:
    /** Constructor.

        \param shortcut key sequence to toggle window visibility
    */
    ChannelSelectorWindow(int shortcut);

    QComboBox* getFoundVideo() const { return foundVideo_; }

    QComboBox* getFoundBinary() const { return foundBinary_; }

    QComboBox* getFoundExtractions() const { return foundExtractions_; }

    QComboBox* getFoundRangeTruths() const { return foundRangeTruths_; }

    QComboBox* getFoundBugPlots() const { return foundBugPlots_; }

private:
    QComboBox* foundVideo_;
    QComboBox* foundBinary_;
    QComboBox* foundExtractions_;
    QComboBox* foundRangeTruths_;
    QComboBox* foundBugPlots_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
