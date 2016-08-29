#include "App.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ExtractionEmitter;

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    return new MainWindow;
}
