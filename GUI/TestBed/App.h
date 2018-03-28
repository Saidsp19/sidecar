#ifndef SIDECAR_GUI_TESTBED_APP_H // -*- C++ -*-
#define SIDECAR_GUI_TESTBED_APP_H

#include "GUI/AppBase.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

/** Namespace for the TestBed application.
 */
namespace TestBed {

class MainWindow;

/** Application class definition.
 */
class App : public AppBase {
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

private:
    /** Override of AppBase method. Create a MainWindow object after the applicadtion has finished starting up.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);
};

} // namespace TestBed
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
