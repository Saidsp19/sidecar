#include "App.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::SignalGenerator;

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    return new MainWindow;
}
