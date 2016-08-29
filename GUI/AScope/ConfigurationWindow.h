#ifndef SIDECAR_GUI_ASCOPE_CONFIGURATIONWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_CONFIGURATIONWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ConfigurationWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace AScope {

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

private slots:

    void on_distanceUnitsKm__toggled(bool);
    void on_distanceUnitsNm__toggled(bool);
    void on_angleFormatMinSec__toggled(bool);
    void on_angleFormatDecimal__toggled(bool);
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
