#include "App.h"

using namespace SideCar::GUI::HealthAndStatus;

int
main(int argc, char** argv)
{
    App* app = new App(argc, argv);
    app->restoreWindows();
    return app->exec();
}
