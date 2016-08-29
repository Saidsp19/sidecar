#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "Decimator.h"
#include "Decimator_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
Decimator::Decimator(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      isIQ_(Parameter::BoolValue::Make("isIQ", "Is the data composed of IQ values", kDefaultIsIQ)),
      factor_(Parameter::PositiveIntValue::Make("factor", "Decimation factor", kDefaultFactor))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the Decimator
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
Decimator::startup()
{
    registerProcessor<Decimator,Messages::Video>(&Decimator::processInput);
    bool ok = true;
    ok = ok 
	&& registerParameter(isIQ_)
	&& registerParameter(factor_)
	&& registerParameter(enabled_);

    return ok && Super::startup();
}

bool
Decimator::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
Decimator::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    // If not enabled, simply pass message.
    //
    if (! enabled_->getValue()) {
	return send(msg);
    }

    // Create a new message to hold the output of what we do. Note that although we pass in the input message, the new
    // message does not contain any data.
    //
    Messages::Video::Ref out(Messages::Video::Make("Decimator::processInput", msg));
    Messages::Video::const_iterator pos = msg->begin();
    Messages::Video::const_iterator end = msg->end();

    int step = factor_->getValue();
    bool isIQ = isIQ_->getValue();

    // Perform the actual decimation.
    //
    if (isIQ) {
	int inc = step * 2 - 1;
	while (pos < end) {
	    out->push_back(*pos++);
	    out->push_back(*pos);
	    pos += inc;
	}
    }
    else {
	while (pos < end) {
	    out->push_back(*pos);
	    pos += step;
	}
    }

    // Update the range factor for the decimated message to reflect the new distance between range bins.
    //
    out->getRIUInfo().rangeFactor *= step;

    // Send out the decimated message.
    //
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
Decimator::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[Decimator::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the Decimator class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
DecimatorMake(Controller& controller, Logger::Log& log)
{
    return new Decimator(controller, log);
}
