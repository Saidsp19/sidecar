#ifndef SIDECAR_GUI_BSCOPE_APP_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_APP_H

#include "QtCore/QList"
#include "QtGui/QImage"

#include "GUI/AppBase.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class ChannelSelectorWindow;
class ControlsWindow;
class PresetsWindow;

/** Namespace for the BScope application.

    \image html BScope.png
*/
namespace BScope {

class Configuration;
class ConfigurationWindow;
class History;
class FramesWindow;
class MainWindow;
class PlayerWindow;

/** Application class for the Master application. Creates and manages the floating tool windows. There is only
    one instance of this class created (in main.cc) for the life of the application.
*/
class App : public AppBase {
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

    QAction* getToolsMenuAction(ToolsMenuAction index) { return Super::getToolsMenuAction(index); }

    /** Obtain the ChannelSelectorWindow tool window.

        \return ChannelSelectorWindow object
    */
    ChannelSelectorWindow* getChannelSelectorWindow() const { return channelSelectorWindow_; }

    /** Obtain the ConfigurationWindow tool window.

        \return ConfigurationWindow object
    */
    ConfigurationWindow* getConfigurationWindow() const { return configurationWindow_; }

    /** Obtain the ControlsWindow tool window.

        \return ControlsWindow object
    */
    ControlsWindow* getControlsWindow() const { return controlsWindow_; }

    /** Obtain the InfoWindow tool window.

        \return InfoWindow object
    */
    PresetsWindow* getPresetsWindow() const { return presetsWindow_; }

    MainWindow* getMainWindow() const { return mainWindow_; }

    PlayerWindow* getPlayerWindow() const { return playerWindow_; }

    FramesWindow* getFramesWindow() const { return framesWindow_; }

    Configuration* getConfiguration() const { return configuration_; }

    History* getHistory() const { return history_; }

    void restoreWindows();

    void addFrame(const QImage& image);

    void setImageSize(const QSize& size);

signals:

    void newFrameCount(int frameCount);

    void imageSizeChanged(const QSize& size);

    void frameAdded(int activeFrames);

private slots:

    void frameCountChanged(int frameCount);

    void applicationQuit();

private:
    void addViewWindow(MainWindowBase* toolWindow);

    /** Create a new MainWindow object and show to the user. Implements AppBase::makeNewMainWindow() interface.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

    Configuration* configuration_;
    History* history_;
    ChannelSelectorWindow* channelSelectorWindow_;
    ConfigurationWindow* configurationWindow_;
    ControlsWindow* controlsWindow_;
    PresetsWindow* presetsWindow_;
    MainWindow* mainWindow_;
    FramesWindow* framesWindow_;
    PlayerWindow* playerWindow_;
    QList<QImage> past_;
    QImage blank_;
    int activeFrames_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
