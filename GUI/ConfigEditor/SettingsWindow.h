#ifndef SIDECAR_GUI_CONFIGEDITOR_SETTINGSWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGEDITOR_SETTINGSWINDOW_H

#include "GUI/ToolWindowBase.h"

namespace Logger { class Log; }
namespace Ui { class SettingsWindow; }

namespace SideCar {
namespace GUI {
namespace ConfigEditor {

/** Window that shows range and power level limits used by the Visualizer objects
 */
class SettingsWindow : public ToolWindowBase
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    static Logger::Log& Log();

    /** Constructor. Creates and initializes the GUI widgets.

	\param action QAction object that manages window's visibility
    */
    SettingsWindow(int shortcut);

private:

    Ui::SettingsWindow* gui_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
