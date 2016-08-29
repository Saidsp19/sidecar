#ifndef SIDECAR_GUI_CONFIGURATIONWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_CONFIGURATIONWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ConfigurationWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace BScope {

class ConfigurationWindow : public ToolWindowBase,
			    public Ui::ConfigurationWindow
{
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
    void on_videoColorMapEnabled__toggled(bool enabled);
    void on_videoColorMap__currentIndexChanged(int index);

    void updateDistanceUnits(const QString& units);
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
