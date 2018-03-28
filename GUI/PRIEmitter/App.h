#ifndef SIDECAR_GUI_PRIEMITTER_APP_H // -*- C++ -*-
#define SIDECAR_GUI_PRIEMITTER_APP_H

#include "GUI/AppBase.h"

namespace SideCar {
namespace GUI {

/** Namespace for the PRIEmitter application.

    \image html PRIEmitter.png

    The PRIEmitter application emits data from a file at a fixed frequency. Supports sending data over TCP and UDP
    (multicast) transports. Note that much of the functionality of this application has been transplanted to the
    SideCar::GUI::Playback utility, which supports emission of data from more than one file at a time.

    It is kept around for sentimental reasons -- it was the first SideCar Qt application.
*/
namespace PRIEmitter {

class MainWindow;

/** Application class for the PRIEmitter application.
 */
class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    /** Obtain the singleton App object.

        \return App instance
    */
    static App* GetApp() { return dynamic_cast<App*>(qApp); }

    /** Constructor.

        \param argc command-line argument count

        \param argv command-line argument values
    */
    App(int& argc, char** argv) : AppBase("PRIEmitter", argc, argv) {}

protected:
    /** Create a new MainWindow object and show to the user. Implements AppBase::makeNewMainWindow() interface.

        \return new MainWindow object
    */
    MainWindowBase* makeNewMainWindow(const QString& objectName);
};

} // namespace PRIEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
