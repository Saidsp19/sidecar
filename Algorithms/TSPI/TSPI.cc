#include "QtCore/QString"

#include "IO/MessageManager.h"
#include "Logger/Log.h"

#include "TSPI.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

TSPI::TSPI(Controller& controller, Logger::Log& log) : Algorithm(controller, log)
{
    ;
}

bool
TSPI::startup()
{
    registerProcessor<TSPI, Messages::TSPI>(&TSPI::process);
    return Algorithm::startup();
}

bool
TSPI::process(const Messages::TSPI::Ref& in)
{
    return send(in);
}

extern "C" ACE_Svc_Export Algorithm*
TSPIMake(Controller& controller, Logger::Log& log)
{
    return new TSPI(controller, log);
}
