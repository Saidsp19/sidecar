#include <fftw3.h>

#include "App.h"
#include "MainWindow.h"

using namespace SideCar::GUI::Spectrum;

int
main(int argc, char** argv)
{
    App* app = new App(argc, argv);
    app->restoreWindows();
    return app->exec();
}
