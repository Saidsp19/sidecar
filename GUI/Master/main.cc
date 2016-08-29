#include "QtGui/QCleanlooksStyle"
#include "QtGui/QPlastiqueStyle"

#include "App.h"

using namespace SideCar::GUI;
using namespace SideCar::GUI::Master;

int
main(int argc, char** argv)
{
#if 0
    QApplication::setStyle(new QPlastiqueStyle);
#endif
    App* app = new App(argc, argv);
    app->restoreWindows();
    return app->exec();
}
