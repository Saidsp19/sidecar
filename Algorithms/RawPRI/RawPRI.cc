#include "QtCore/QString"

#include "boost/bind.hpp"

#include "IO/MessageManager.h"
#include "IO/TaskStatus.h"
#include "Logger/Log.h"
#include "Messages/Video.h"

#include "RawPRI.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

RawPRI::RawPRI(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log)
{
    ;
}

bool
RawPRI::startup()
{
    registerProcessor<RawPRI,Messages::RawVideo>(&RawPRI::process);
    return Algorithm::startup();
}

bool
RawPRI::process(const Messages::RawVideo::Ref& in)
{
    Messages::Video::Ref out(in->convert("RawPRI"));
    return send(out);
}

extern "C" ACE_Svc_Export Algorithm*
RawPRIMake(Controller& controller, Logger::Log& log)
{
    return new RawPRI(controller, log);
}
