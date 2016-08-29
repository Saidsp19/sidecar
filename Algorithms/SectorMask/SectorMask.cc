#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Messages/RadarConfig.h"
#include "Logger/Log.h"

#include "SectorMask.h"
#include "SectorMask_defaults.h"

#include "QtCore/QString"
#include "QtCore/QVariant"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
SectorMask::SectorMask(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make(
                   "enabled", "Enabled", kDefaultEnabled)),
      minAzimuth_(Parameter::DoubleValue::Make(
                      "minAzimuth", "Min azimuth to mask", kDefaultMinAzimuth)),
      maxAzimuth_(Parameter::DoubleValue::Make(
                      "maxAzimuth", "Max azimuth to mask", kDefaultMaxAzimuth)),
      minRangeBin_(Parameter::IntValue::Make(
                       "minRangeBin", "Min range bin to mask", kDefaultMinRangeBin)),
      maxRangeBin_(Parameter::IntValue::Make(
                       "maxRangeBin", "Max range bin to mask", kDefaultMaxRangeBin))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// SectorMask class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
SectorMask::startup()
{
    registerProcessor<SectorMask,Messages::BinaryVideo>(
        &SectorMask::processInput);
    bool ok = true;

    ok = ok 
        && registerParameter(minAzimuth_)
        && registerParameter(maxAzimuth_)
        && registerParameter(minRangeBin_)
        && registerParameter(maxRangeBin_)
        && registerParameter(enabled_);

    return ok && Super::startup();
}

bool
SectorMask::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
SectorMask::NormalizeBins(int& startRngBin, int& endRngBin, size_t msg_size) 
{
    static Logger::ProcLog log("NormalizeBins", getLog());

    if(startRngBin < 0) 
	startRngBin += msg_size;

    if(endRngBin < 0) 
	endRngBin += msg_size;

    if(startRngBin < endRngBin || startRngBin >= msg_size) {
	LOGERROR << "Invalid minRangeBin (" << startRngBin << ") or maxRangeBin "
                 << endRngBin << ") values with message size: " << msg_size << std::endl;
	return false;
    }

    return true;
}

bool
SectorMask::processInput(const Messages::BinaryVideo::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    // Create a new message to hold the output of what we do. Note that although we pass in the input message,
    // the new message does not contain any data.
    //
    Messages::BinaryVideo::Ref out(
        Messages::BinaryVideo::Make("SectorMask::processInput", msg));

    out->getData() = msg->getData();
	
    double azimuth = 360.0 * float(msg->getShaftEncoding()) /
        float(Messages::RadarConfig::GetShaftEncodingMax() + 1);

    int startRngBin = minRangeBin_->getValue();
    int endRngBin   = maxRangeBin_->getValue();
    size_t msg_size = msg->size();

    // Determine if this pulse falls within the targeted sector's azimuth boundaries, if not, just pass
    // message on.
    //
    if(azimuth >= minAzimuth_->getValue() && azimuth <= maxAzimuth_->getValue()) {

        if(!NormalizeBins(startRngBin, endRngBin, msg_size)) 
            return false;

        // Mask out any values in the given sector
        //
        for(size_t i = startRngBin; i <= endRngBin; i++) {
            out[i] = false;
        }
    }

    // Send out on the default output device, and return the result to our Controller. NOTE: for multichannel
    // output, one must give a channel index to the send() method. Use getOutputChannelIndex() to obtain the
    // index for an output channel with a given name.
    //

    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

void
SectorMask::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export QVariant
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (! status[SectorMask::kEnabled]) return "Disabled";

    // Format status information here.
    //
    return QString("");
}

// Factory function for the DLL that will create a new instance of the SectorMask class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SectorMaskMake(Controller& controller, Logger::Log& log)
{
    return new SectorMask(controller, log);
}
