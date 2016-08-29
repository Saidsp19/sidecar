#ifndef SIDECAR_GUI_LOGGERWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_LOGGERWINDOW_H

#include "QtCore/QModelIndex"

#include "GUI/ToolWindowBase.h"

namespace Logger { class Log; }
namespace Ui { class LoggerWindow; }
namespace SideCar {
namespace GUI {

class LoggerModel;

/** Floating tool window that shows the various Logger::Log devices in use by a GUI program. Lets the user
    change priority levels for the devices.
*/
class LoggerWindow : public ToolWindowBase
{
    Q_OBJECT
    using Super = ToolWindowBase;
public:

    /** Constructor. Creates and initializes window widgets.

        \param shortcut the key sequence that controls window visibility
    */
    LoggerWindow(int shortcut);

private:
    Ui::LoggerWindow* gui_;
    LoggerModel* model_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
