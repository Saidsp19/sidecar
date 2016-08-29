#ifndef SIDECAR_GUI_MASTER_CONFIGURATIONWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_CONFIGURATIONWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ConfigurationWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace Master {

/** Window that shows range and power level limits used by the Visualizer objects
 */
class ConfigurationWindow : public ToolWindowBase,
			    public Ui::ConfigurationWindow
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    static Logger::Log& Log();

    /** Constructor. Creates and initializes the GUI widgets.

	\param action QAction object that manages window's visibility
    */
    ConfigurationWindow(int shortcut);
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
