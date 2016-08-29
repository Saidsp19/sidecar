#include "App.h"
#include "MainWindow.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::ZeroconfMonitor;

int
main(int argc, char** argv)
{
    App* app = new App(argc, argv);
    app->restoreWindows();
    return app->exec();
}
