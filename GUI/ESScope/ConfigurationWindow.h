#ifndef SIDECAR_GUI_CONFIGURATIONWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGURATIONWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ConfigurationWindow.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class MessageWriter;

namespace ESScope {

class ConfigurationWindow : public ToolWindowBase, public Ui::ConfigurationWindow {
    Q_OBJECT
    using Super = ToolWindowBase;

public:
    static Logger::Log& Log();

    ConfigurationWindow(int shortcut);

private slots:

    void on_distanceUnitsKm__toggled(bool);
    void on_distanceUnitsNm__toggled(bool);
    void on_angleFormatMinSec__toggled(bool);
    void on_angleFormatDecimal__toggled(bool);
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
