#ifndef SIDECAR_GUI_MASTER_APP_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_APP_H

#include "QtCore/QDir"

#include "GUI/AppBase.h"

namespace SideCar {
namespace GUI {

/** Namespace for the Master application.

    \image html Master.png

    The Master application manages the SideCar processing streams that run on
    in the servers in the SideCar LAN. Users load in various XML configuration
    files and start up the processing streams they define. The processing
    streams exist inside a SideCar::Runner application. The SideCar::Runner
    application periodically emits status information which the Master
    application presents to the user as a set of hierarchical rows, ordered by
    host and stream name.

    Users may change an algorithm's runtime parameters at any time by clicking
    on an algorithm entry in the main display. The Master application submits
    parameter changes in an XML-RPC request, which is handled by the remote
    SideCar::Runner task with its internal XML-RPC server.

    Finally, the Master application can command remote Runner applications to
    start and stop data recording, again using XML-RPC requests. These
    recordings may be played back at some later time by using the
    SideCar::GUI::Playback application.

    <h2>Basic User Guide</h2>

    Write me.
*/

namespace Master {

class ConfigurationWindow;
class ConfigurationSettings;
class LogAlertsWindow;
class MainWindow;
class RadarSettings;
class Settings;

/** Application class for the Master Controller application.
 */
class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    enum ToolsMenuAction { kShowConfigurationWindow, kShowLogAlertsWindow, kNumToolsMenuActions };

    /** Obtain type-casted App singleton object.

        \return App object
    */
    static App* GetApp() { return static_cast<App*>(AppBase::GetApp()); }

    App(int& argc, char** argv);

    ~App();

    QAction* getToolsMenuAction(ToolsMenuAction index) { return Super::getToolsMenuAction(index); }

    Settings& getSettings() const { return *settings_; }

    /** Obtain the configuration tool window

        \return ConfigurationWindow window
    */
    ConfigurationWindow* getConfigurationWindow() const { return configurationWindow_; }

    ConfigurationSettings& getConfigurationSettings() const;

    RadarSettings& getRadarSettings() const;

    LogAlertsWindow* getLogAlertsWindow() const { return logAlertsWindow_; }

    void setRecording(bool state) { recording_ = state; }

    bool isRecording() const { return recording_; }

    void closeAuxillaryWindows() { emit closingAuxillaryWindows(); }

    void restoreWindows();

    MainWindow* getMainWindow() const { return mainWindow_; }

    const QDir& getWorkingDirectory() const { return workingDirectory_; }

signals:

    void closingAuxillaryWindows();

public slots:

    void applicationQuit();

private:
    /** Create the tool windows.
     */
    void makeToolWindows();

    /** Override of AppBase method. Creates a new MainWindow object.

        \return MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

    ConfigurationWindow* configurationWindow_;
    QDir workingDirectory_;
    MainWindow* mainWindow_;
    LogAlertsWindow* logAlertsWindow_;
    Settings* settings_;
    bool recording_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
