#ifndef SIDECAR_GUI_ESSCOPE_APP_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_APP_H

#include "QtCore/QList"
#include "QtGui/QImage"

#include "GUI/AppBase.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class ChannelSelectorWindow;
class ControlsWindow;
class PresetsWindow;

/** Namespace for the ESScope application.

    \image html ESScope.png
*/
namespace ESScope {

class Configuration;
class ConfigurationWindow;
class MainWindow;
class ViewEditor;

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
	kShowViewEditor,
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

    Configuration* getConfiguration() const { return configuration_; }

    ChannelSelectorWindow* getChannelSelectorWindow() const
	{ return channelSelectorWindow_; }

    ConfigurationWindow* getConfigurationWindow() const
	{ return configurationWindow_; }

    ControlsWindow* getControlsWindow() const { return controlsWindow_; }

    PresetsWindow* getPresetsWindow() const { return presetsWindow_; }

    ViewEditor* getViewEditor() const { return viewEditor_; }

    MainWindow* getMainWindow() const { return mainWindow_; }

private slots:

    void applicationQuit();

private:

    void makeToolWindows();

    MainWindowBase* makeNewMainWindow(const QString& objectName);

    Configuration* configuration_;
    ChannelSelectorWindow* channelSelectorWindow_;
    ConfigurationWindow* configurationWindow_;
    ControlsWindow* controlsWindow_;
    PresetsWindow* presetsWindow_;
    ViewEditor* viewEditor_;
    MainWindow* mainWindow_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
