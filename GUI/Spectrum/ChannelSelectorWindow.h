#ifndef SIDECAR_GUI_SPECTRUM_CHANNELSELECTORWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_CHANNELSELECTORWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ChannelSelectorWindow.h"

namespace SideCar {
namespace GUI {

class ChannelGroup;

namespace Spectrum {

/** Tool window that shows three QComboBox buttons that allow the user to select which Video, BinaryVideo, and
    Extractions publisher to connect to for data. Most of the work is done from inside the ChannelGroup class;
    this class simply establishes connections between the GUI and the ChannelGroup instances.
*/
class ChannelSelectorWindow : public ToolWindowBase,
			      public Ui::ChannelSelectorWindow
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    /** Constructor.

	\param shutdown key sequence to toggle window visibility
    */
    ChannelSelectorWindow(int shortcut);
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
