#include "App.h"

using namespace SideCar::GUI::TestBed;

int
main(int argc, char** argv)
{
    App* app = new App(argc, argv);
    app->setStyleSheet("* { background-color: qlineargradient("
                       "x1: 0.0, y1: 0.0, "
                       "x2: 1.0, y2: 0.0, "
                       "stop:0.0 gray, "
                       "stop:0.5 white "
                       "stop:1.0 gray) }");
    app->restoreWindows();
    return app->exec();
}
