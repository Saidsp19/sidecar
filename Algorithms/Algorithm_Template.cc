#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Logger/Log.h"

#include "@@NAME@@.h"
#include "@@NAME@@_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method.
//
@@NAME@@::@@NAME@@(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
      exampleParameter_(Parameter::IntValue::Make("example", "Example Label", kDefaultExample))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// @@NAME@@ class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
@@NAME@@::startup()
{
    // Register message processor. Change as needed.
    //
    registerProcessor<@@NAME@@,Messages::Video>(&@@NAME@@::process);

    // Register runtime parameters, and invoke parent class startup() method.
    //
    return registerParameter(exampleParameter_) && Algorithm::startup();
}

bool
@@NAME@@::process(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());

    // Create a new message to hold the output of what we do. Note that although we pass in the input message,
    // the new message does not contain any data.
    //
    Messages::Video::Ref out(Messages::Video::Make("@@NAME@@", msg));

    // Example of using the STL transform function to do looping for us. Here we are add 100 to all values.
    //
    out->resize(msg->size());
    std::transform(msg->begin(), msg->end(), out->begin(), [](auto v){return v + 100;});

    // Transformation is done -- send out on the default output device, and return the result to our Controller.
    //
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

// Factory function for the DLL that will create a new instance of the @@NAME@@ class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
@@NAME@@Make(Controller& controller, Logger::Log& log)
{
    return new @@NAME@@(controller, log);
}
