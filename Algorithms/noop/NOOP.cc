#include "Logger/Log.h"

#include "NOOP.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

NOOP::NOOP(Controller& controller, Logger::Log& log) : Algorithm(controller, log)
{
    ;
}

bool
NOOP::startup()
{
    registerProcessor<NOOP, Messages::Video>(&NOOP::processVideo);
    return Algorithm::startup();
}

bool
NOOP::processVideo(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processVideo", getLog());
    LOGINFO << std::endl;
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->getData() = msg->getData();
    return send(out);
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    return NULL;
}

extern "C" ACE_Svc_Export Algorithm*
NOOPMake(Controller& controller, Logger::Log& log)
{
    return new NOOP(controller, log);
}
