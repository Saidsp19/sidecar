#ifndef SIDECAR_GUI_PLAYBACK_APP_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_APP_H

#include "GUI/AppBase.h"

namespace SideCar {
namespace GUI {

/** Namespace for the Playback application.

    \image html Playback.png

    The SideCar Playback application can read binary files created by SideCar
    algorithms when they had their recorders turned on. Unlike the PRIEmitter
    application, Playback can retransmit data from multiple files at the same
    time. It also supports random access within the recording files, jumping
    forwards and backwards in time using both relative and absolute times.
*/
namespace Playback {

class BrowserWindow;

/** Application class for the Playback application.
 */
class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    /** Menu action enumeration for tool window show/hide actions.
     */
    enum ToolsMenuAction { kShowBrowserWindow, kNumToolsMenuActions };

    /** Obtain type-casted App singleton object.

        \return App object
    */
    static App* GetApp() { return static_cast<App*>(AppBase::GetApp()); }

    /** Constructor

        \param argc number of arguments on the command line

        \param argv array of command line parameter values
    */
    App(int& argc, char** argv);

    QAction* getToolsMenuAction(ToolsMenuAction index) { return Super::getToolsMenuAction(index); }

    BrowserWindow* getBrowserWindow() const { return browserWindow_; }

private:
    /** Override of AppBase method. Creates a new MainWindow object.

        \return MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

    BrowserWindow* browserWindow_;
};

} // end namespace Playback
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
