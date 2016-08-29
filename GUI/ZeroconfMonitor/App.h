#ifndef SIDECAR_GUI_ZEROCONFMONITOR_APP_H // -*- C++ -*-
#define SIDECAR_GUI_ZEROCONFMONITOR_APP_H

#include "QtCore/QList"

#include "GUI/AppBase.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

/** Namespace for the ZeroconfMonitor application.
    
    \image html ZeroconfMonitor.png

    The ZeroconfMonitor (zcm) application shows active Zeroconf publishers
    running in the SideCar LAN. The zcm window contains four widgets that show
    connections for the following processes:

    - <b>Publishers</b> - active data publishers sending data using the TCP or
    UDP transport.
    - <b>Runner Remote Controllers</b> - active runner processes. The
    connection information presented is the host/port running the XML-RPC
    server for the runner process.
    - <b>Runner Status Collectors</b> - active Master processes which listen
    for status information sent out by runner processes.
*/
namespace ZeroconfMonitor {

class MainWindow;

/** Application class definition. Manages tools windows shared by all MainWindow objects. Also creates a History
    object that other classes use to obtain Video data for display.
*/
class App : public AppBase
{
    Q_OBJECT
    using Super = AppBase;
public:

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

public slots:

    void showAbout();

protected:
    
    /** Override of AppBase method. Create a MainWindow object after the applicadtion has finished starting up.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);

private:
};

} // end namespace ZeroconfMonitor
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
