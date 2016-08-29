#ifndef SIDECAR_GUI_SPECTRUM_CONFIGURATIONWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_CONFIGURATIONWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ConfigurationWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class IntMinMaxValidator;

namespace Spectrum {

/** Window that shows range and power level limits used by the Visualizer and RenderThread objects
 */
class ConfigurationWindow : public ToolWindowBase,
			    public Ui::ConfigurationWindow
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    /** Constructor. Creates and initializes the GUI widgets.

	\param shortcut key sequence to toggle window visibility
    */
    ConfigurationWindow(int shortcut);

private:
    IntMinMaxValidator* validator_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
