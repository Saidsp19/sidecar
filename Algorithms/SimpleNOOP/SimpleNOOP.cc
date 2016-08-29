#include "Logger/Log.h"

#include "SimpleNOOP.h"
#include "SimpleNOOP_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method.
//
SimpleNOOP::SimpleNOOP(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log)
{
    ;
}

bool
SimpleNOOP::startup()
{
    registerProcessor<SimpleNOOP,Messages::Video>(&SimpleNOOP::process);
    return Algorithm::startup();
}

bool
SimpleNOOP::process(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());
    
    if (msg->size() > 30 * 1024) {
	LOGERROR << "dropping abnormally large message: " << msg->size() << std::endl;
	return true;
    }

    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->getData() = msg->getData();
    return send(out);
}

// Factory function for the DLL that will create a new instance of the SimpleNOOP class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SimpleNOOPMake(Controller& controller, Logger::Log& log)
{
    return new SimpleNOOP(controller, log);
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    return NULL;
}
