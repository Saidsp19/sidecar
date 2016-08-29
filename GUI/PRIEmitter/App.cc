#include "App.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::PRIEmitter;

MainWindowBase*
App::makeNewMainWindow(const QString& objectName)
{
    MainWindow* window = new MainWindow;
    QStringList args = arguments();
    if (args.size() > 1) {
	window->openFile(args.at(1));
    }
    return window;
}
