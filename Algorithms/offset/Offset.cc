#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/Video.h"

#include "Utils/VsipVector.h"
#include <vsip/math.hpp>

#include "Offset.h"
#include "Offset_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

Offset::Offset(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled",
                                          kDefaultEnabled)),
      offset_(Parameter::ShortValue::Make("offset", "Offset",
                                          kDefaultOffset))
{
    ;
}

bool
Offset::startup()
{
    registerProcessor<Offset,Messages::Video>(&Offset::process);
    return registerParameter(enabled_) &&
	registerParameter(offset_) &&
	Algorithm::startup();
}

bool
Offset::process(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());

    if (! enabled_->getValue())
	return send(msg);

    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->resize(msg->size());
    VsipVector<Messages::Video> vIn(*msg);
    VsipVector<Messages::Video> vOut(*out);
    vIn.admit(true);
    vOut.admit(false);
    vOut.v = vIn.v + offset_->getValue();
    vIn.release(false);
    vOut.release(true);
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

extern "C" ACE_Svc_Export Algorithm*
OffsetMake(Controller& controller, Logger::Log& log)
{
    return new Offset(controller, log);
}
