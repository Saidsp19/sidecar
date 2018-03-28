#ifndef SIDECAR_GUI_EXTRACTIONEMITTER_APP_H // -*- C++ -*-
#define SIDECAR_GUI_EXTRACTIONEMITTER_APP_H

#include "QtCore/QList"

#include "GUI/AppBase.h"

namespace SideCar {
namespace GUI {

/** Namespace for the ExtractionEmitter application.

    \image html ExtractionEmitter.png

    The ExtractionEmitter application emits Messages::Extractions messages. It
    was originally created to test extraction plotting in the GUI::PPIDisplay
    application before the Algorithms::Extract algorithm was available.

    Now that there is a ready source of extractions from that algorithm, this
    application should probably be retired and deleted from the code tree.
*/
namespace ExtractionEmitter {

class App : public AppBase {
    Q_OBJECT
    using Super = AppBase;

public:
    static App* GetApp() { return dynamic_cast<App*>(qApp); }

    App(int& argc, char** argv) : AppBase("ExtractionsEmitter", argc, argv) {}

protected:
    MainWindowBase* makeNewMainWindow(const QString& objectName);
};

} // end namespace ExtractionEmitter
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
