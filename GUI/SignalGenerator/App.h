#ifndef SIDECAR_GUI_SIGNALGENERATOR_APP_H // -*- C++ -*-
#define SIDECAR_GUI_SIGNALGENERATOR_APP_H

#include "QtCore/QList"

#include "GUI/AppBase.h"

namespace SideCar {
namespace GUI {

/** Namespace for the SignalGenerator application.

    \image html SignalGenerator.png

    The SignalGenerator application emits Messages::Video messages.
*/
namespace SignalGenerator {

class App : public AppBase
{
    Q_OBJECT
    using Super = AppBase;
public:
    static App* GetApp() { return dynamic_cast<App*>(qApp); }

    App(int& argc, char** argv)
	: AppBase("SignalGenerator", argc, argv) {}

protected:
    MainWindowBase* makeNewMainWindow(const QString& objectName);
};

} // end namespace SignalGenerator
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
