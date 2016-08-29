#ifndef SIDECAR_GUI_HEALTHANDSTATUS_CONFIGURATIONWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_CONFIGURATIONWINDOW_H

#include "GUI/ToolWindowBase.h"

#include "ui_ConfigurationWindow.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace HealthAndStatus {

class ChannelPlotSettings;

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

    void addChannelPlotSettings(ChannelPlotSettings* settings);

    void removeChannelPlotSettings(ChannelPlotSettings* settings);

    ChannelPlotSettings* getDefaultSettings() const { return defaults_; }

    void useDefaultSettings() { updateFromSettings(defaults_); }

private slots:

    void on_updateAll__clicked();

    void on_updateExpected__clicked();

private:

    void updateButtons();

    void updateFromSettings(ChannelPlotSettings* settings);

    ChannelPlotSettings* defaults_;
    QList<ChannelPlotSettings*> connected_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
