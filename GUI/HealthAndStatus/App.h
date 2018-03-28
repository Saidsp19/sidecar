#ifndef SIDECAR_GUI_HEALTHANDSTATUS_APP_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_APP_H

#include "GUI/AppBase.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class PresetManager;

/** Namespace for the HealthAndStatus (hands) application.

    \image html HealthAndStatus.png

    The HealthAndStatus application shows various displays and plots to convey
    to the user how well the SideCar system is performing.
*/
namespace HealthAndStatus {

class ConfigurationWindow;
class MainWindow;

/** Application class definition. Manages tool windows shared by all MainWindow objects.
 */
class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    enum ToolsMenuAction { kShowConfigurationWindow, kNumToolsMenuActions };

    /** Log device to use for App log messages.

        \return Log device
    */
    static Logger::Log& Log();

    /** Obtain the App singleton.

        \return App object
    */
    static App* GetApp() { return dynamic_cast<App*>(qApp); }

    /** Constructor.

        \param argc argument count from the command line

        \param argv argument values from the command line
    */
    App(int& argc, char** argv);

    QAction* getToolsMenuAction(ToolsMenuAction index) { return Super::getToolsMenuAction(index); }

    PresetManager* getPresetManager() const { return presetManager_; }

    /** Obtain the configuration tool window

        \return ConfigurationWindow window
    */
    ConfigurationWindow* getConfigurationWindow() const { return configurationWindow_; }

    MainWindow* getMainWindow() const { return mainWindow_; }

    void applicationQuit();

private:
    /** Override of AppBase method. Create a MainWindow object after the applicadtion has finished starting up.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

    PresetManager* presetManager_;
    ConfigurationWindow* configurationWindow_;
    MainWindow* mainWindow_;
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
