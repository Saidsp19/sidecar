#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/Video.h"

#include "Utils/VsipVector.h"

#include "Scale.h"
#include "Scale_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

Scale::Scale(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled",
                                          kDefaultEnabled)),
      scale_(Parameter::DoubleValue::Make("scale", "Scale",
                                          kDefaultScale))
{
    ;
}

bool
Scale::startup()
{
    registerProcessor<Scale,Messages::Video>(&Scale::process);
    return registerParameter(enabled_) &&
	registerParameter(scale_) &&
	Algorithm::startup();
}

bool
Scale::process(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());

    if (! enabled_->getValue())
	return send(msg);

    // Obtain a new message to use for our output values.
    //
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));
    out->resize(msg->size());

    // Use VSIPL to do the conversion for us. First, we need to create VSIPL vectors that use our message data.
    //
    VsipVector<Messages::Video> vMsg(*msg);
    VsipVector<Messages::Video> vOut(*out);

    vMsg.admit(true);		// Tell VSIPL to use data from msg
    vOut.admit(false);	// Tell VSIPL to ignore data from out

    // Perform the scaling.
    //
    vOut.v = scale_->getValue() * vMsg.v;

    vMsg.release(false);	// Don't flush data from VSIPL to msg
    vOut.release(true);	// Do flush data from VSIPL to out

    // Send out on the default output channel, and return the result to the controller.
    //
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

extern "C" ACE_Svc_Export Algorithm*
ScaleMake(Controller& controller, Logger::Log& log)
{
    return new Scale(controller, log);
}
