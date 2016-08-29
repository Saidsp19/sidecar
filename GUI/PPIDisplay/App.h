#ifndef SIDECAR_GUI_PPIDISPLAY_APP_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_APP_H

#include "GUI/AppBase.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class ChannelSelectorWindow;
class PresetsWindow;

/** Namespace for the PPIDisplay application.

    \image html PPIDisplay.png

    The PPIDisplay application simulates a plot position indicator (PPI)
    display seen in old movies and the like, with a green or orange phosphor
    that decays with time as the radar rotates around. This simulation looks
    nice but it in no way simulates actual phosphor decay, nor does it make any
    attempts to accurately emulate an existing PPI display.

    In order to run the PPIDisplay, a system must have a moderately powerful
    graphics card that supports OpenGL with framebuffer object (FBO) support.
    It runs fine at low (~80 Hz) data rates on a PowerBook 1.5GHz laptop with
    an ATI Mobility Radeon 9700 display card, but it screems on an NVIDIA
    Quadro FX 600 at 350 Hz data rate, with 250+ frames per second update rate.
*/

namespace PPIDisplay {

class Configuration;
class ConfigurationWindow;
class ControlsWindow;
class History;

/** Application class for the Master application. Creates and manages the floating tool windows. There is only
    one instance of this class created (in main.cc) for the life of the application.
*/
class App : public AppBase
{
    Q_OBJECT
    using Super = AppBase;
public:

    enum ToolsMenuAction {
	kShowChannelSelectorWindow,
	kShowConfigurationWindow,
	kShowControlsWindow,
	kShowPresetsWindow,
	kNumToolsMenuActions
    };

    /** Obtain the Log device for App instances

        \return Log device
    */
    static Logger::Log& Log();

    /** Obtain the singleton App object.

        \return App instance
    */
    static App* GetApp() { return dynamic_cast<App*>(qApp); }

    /** Constructor. Creates floating tool windows.

        \param argc command-line argument count

        \param argv vector of command-line argument values
    */
    App(int& argc, char** argv);

    QAction* getToolsMenuAction(ToolsMenuAction index)
	{ return Super::getToolsMenuAction(index); }

    /** Obtain the ChannelSelectorWindow tool window.

        \return ChannelSelectorWindow object
    */
    ChannelSelectorWindow* getChannelSelectorWindow() const
	{ return channelSelectorWindow_; }

    /** Obtain the ConfigurationWindow tool window.

        \return ConfigurationWindow object
    */
    ConfigurationWindow* getConfigurationWindow() const
	{ return configurationWindow_; }

    /** Obtain the ControlsWindow tool window.

        \return ControlsWindow object
    */
    ControlsWindow* getControlsWindow() const
	{ return controlsWindow_; }

    /** Obtain the PresetsWindow tool window.

        \return PresetsWindow object
    */
    PresetsWindow* getPresetsWindow() const
	{ return presetsWindow_; }

    Configuration* getConfiguration() const { return configuration_; }

    History* getHistory() const { return history_; }

private slots:

    void applicationQuit();
	
protected:

    /** Create a new MainWindow object and show to the user. Implements AppBase::makeNewMainWindow() interface.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

private:
    
    /** Create the tool windows.
     */
    void makeToolWindows();

    Configuration* configuration_;
    History* history_;
    ChannelSelectorWindow* channelSelectorWindow_;
    ConfigurationWindow* configurationWindow_;
    ControlsWindow* controlsWindow_;
    PresetsWindow* presetsWindow_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
