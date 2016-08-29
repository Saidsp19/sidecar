#include "ace/Thread_Manager.h"

#include "App.h"

using namespace SideCar::Runner;

int
main(int argc, char** argv)
{
    // Allocate global ACE thread manager.
    //
    ACE_Thread_Manager::instance();

    // Initialize runner Application class and start up.
    //
    App app(argc, argv);
    app.initialize();
    app.run();

    return 0;
}
