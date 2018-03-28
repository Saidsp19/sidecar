#ifndef SIDECAR_GUI_RANGETRUTHEMITTER_APP_H // -*- C++ -*-
#define SIDECAR_GUI_RANGETRUTHEMITTER_APP_H

#include "QtCore/QList"

#include "GUI/AppBase.h"

namespace SideCar {
namespace GUI {

/** Namespace for the RangeTruthEmitter application.

    \image html RangeTruthEmitter.png

    The RangeTruthEmitter application emits Messages::RangeTruths messages. It
    was created to test range truth plotting in the GUI::PPIDisplay
    application.
*/
namespace RangeTruthEmitter {

class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    static App* GetApp() { return dynamic_cast<App*>(qApp); }

    App(int& argc, char** argv) : AppBase("RangeTruthEmitter", argc, argv) {}

protected:
    MainWindowBase* makeNewMainWindow(const QString& objectName);
};

} // end namespace RangeTruthEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
