#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Logger/Log.h"

#include "Volts2Power.h"
#include "Volts2Power_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method.
//
Volts2Power::Volts2Power(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log)
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// Volts2Power class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
Volts2Power::startup()
{
    // Register message processor. Change as needed.
    //
    registerProcessor<Volts2Power,Messages::Video>(&Volts2Power::process);

    return true;
}

bool
Volts2Power::process(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());

    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->reserve(msg->size() / 2);
    Messages::Video::const_iterator pos = msg->begin();
    Messages::Video::const_iterator end = msg->end();

    while (pos < end) {
	float r = *pos++;
	float i = *pos++;
	float v = 10.0 * ::log10((r * r + i * i) * 2.5);
	out->push_back(Messages::Video::DatumType(::rintf(v)));
    };

    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

// Factory function for the DLL that will create a new instance of the Volts2Power class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
Volts2PowerMake(Controller& controller, Logger::Log& log)
{
    return new Volts2Power(controller, log);
}
