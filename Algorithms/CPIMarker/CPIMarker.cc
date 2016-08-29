#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "CPIMarker.h"
#include "CPIMarker_defaults.h"

#include "QtCore/QString"

#include "Time/TimeStamp.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
CPIMarker::CPIMarker(Controller& controller, Logger::Log& log)
    : Super(controller, log), last_(0),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      scale_(Parameter::PositiveIntValue::Make("scale", "Scaling factor", kDefaultScale)),
      freqMHz_(Parameter::DoubleValue::Make("freqMHz", "Frequency (in MHz) of radar", kDefaultFreqMHz))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the CPIMarker
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
CPIMarker::startup()
{
    registerProcessor<CPIMarker,Messages::Video>(&CPIMarker::processInput);
    bool ok = true;
    ok = ok 
        && registerParameter(scale_)
        && registerParameter(freqMHz_)
        && registerParameter(enabled_);
    return ok && Super::startup();
}

bool
CPIMarker::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
CPIMarker::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    if(!last_) {
        last_ = Messages::Video::Make("CPIMarker::processInput", msg);
        return true;
    }

#if 1
    uint32_t delta   = msg->getRIUInfo().timeStamp - 
        last_->getRIUInfo().timeStamp + 1;
    uint32_t prfFreq = uint32_t(delta / 1000);

    LOGDEBUG << "Msg: " << msg->getRIUInfo().sequenceCounter << " has delta = "
             << delta << " and prfFreq = " << prfFreq << std::endl;

#else

    uint32_t prfFreq = msg->size() / scale_->getValue();

    LOGDEBUG << "Msg: " << msg->getRIUInfo().sequenceCounter << " has size = "
             << msg->size() << " and prfFreq = " << prfFreq << std::endl;

#endif

    msg->getRIUInfo().prfEncoding = prfFreq;

    last_ = Messages::Video::Make("CPIMarker::processInput", msg);
	
    // Send out on the default output device, and return the result to our Controller. NOTE: for multichannel output,
    // one must give a channel index to the send() method. Use getOutputChannelIndex() to obtain the index for an
    // output channel with a given name.
    //
    bool rc = send(msg);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
CPIMarker::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[CPIMarker::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return Algorithm::FormatInfoValue("");
}

// Factory function for the DLL that will create a new instance of the CPIMarker class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
CPIMarkerMake(Controller& controller, Logger::Log& log)
{
    return new CPIMarker(controller, log);
}
