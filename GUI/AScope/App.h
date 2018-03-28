#ifndef SIDECAR_GUI_ASCOPE_APP_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_APP_H

#include "QtCore/QList"

#include "GUI/AppBase.h"

#include "Configuration.h"

class QPrinter;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

/** Namespace for the AScope application.

    \image html AScope.png

    The AScope application is a simple software simulation of the venerable
    oscilloscope. It accepts incoming data from one or more channels containing
    Messages::Video data. The application supports multiple windows, each with
    one or more independent views of the incoming data. It also provides
    zooming and panning of the display view.
*/
namespace AScope {

class ChannelConnectionWindow;
class ConfigurationWindow;
class DefaultViewSettings;
class DisplayView;
class History;
class HistorySettings;
class MainWindow;
class PeakBarSettings;
class ViewEditor;

/** Application class definition. Manages tool windows shared by all MainWindow objects. Also creates a History
    object that other classes use to obtain Video data for display.
*/
class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    /** Menu action enumeration for tool window show/hide actions.
     */
    enum ToolsMenuAction {
        kShowChannelConnectionWindow,
        kShowConfigurationWindow,
        kShowViewEditor,
        kNumToolsMenuActions
    };

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

    /** Destructor. Deletes the allocated QPrinter.
     */
    ~App();

    QAction* getToolsMenuAction(ToolsMenuAction index) { return Super::getToolsMenuAction(index); }

    History& getHistory() { return *history_; }

    Configuration& getConfiguration() const { return *configuration_; }

    DefaultViewSettings& getDefaultViewSettings() const { return configuration_->getDefaultViewSettings(); }

    HistorySettings& getHistorySettings() const { return configuration_->getHistorySettings(); }

    PeakBarSettings& getPeakBarSettings() const { return configuration_->getPeakBarSettings(); }

    /** Obtain the channel connection tool window

        \return ChannelConnectionWindow window
    */
    ChannelConnectionWindow* getChannelConnectionWindow() const { return channelConnectionWindow_; }

    /** Obtain the configuration tool window

        \return ConfigurationWindow window
    */
    ConfigurationWindow* getConfigurationWindow() const { return configurationWindow_; }

    /** Obtain the configuration tool window

        \return ConfigurationWindow window
    */
    ViewEditor* getViewEditor() const { return viewEditor_; }

    /** Add connections between some of the tool windows and a newly-created DisplayView object.

        \param displayView object to work with
    */
    void addDisplayView(DisplayView* displayView);

    /** Obtain the application's QPrinter object.

        \return QPrinter object
    */
    QPrinter* getPrinter() const { return printer_; }

private:
    /** Override of AppBase method. Create a MainWindow object after the application has finished starting up.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

    /** Create the tool windows.
     */
    void makeToolWindows();

    QPrinter* printer_;
    History* history_;
    ChannelConnectionWindow* channelConnectionWindow_;
    ConfigurationWindow* configurationWindow_;
    ViewEditor* viewEditor_;
    Configuration* configuration_;
    int windowCounter_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
