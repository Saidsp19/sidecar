#include <algorithm>		// for std::copy

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "ShiftBins.h"
#include "ShiftBins_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
ShiftBins::ShiftBins(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      shift_(Parameter::IntValue::Make("shift", "Amount to shift<br>" "> 0 shifts right - &lt; 0 shifts left",
                                       kDefaultShift))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the ShiftBins
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
ShiftBins::startup()
{
    size_t index = 0;
    const IO::Channel& channel(getController().getInputChannel(index));

    // Use the appropriate message handler depending on input type.
    //
    if (channel.getTypeKey() == Video::GetMetaTypeInfo().getKey()) {
	registerProcessor<ShiftBins,Video>(index, &ShiftBins::processInputVideo);
    }
    else if (channel.getTypeKey() == BinaryVideo::GetMetaTypeInfo().getKey()) {
	registerProcessor<ShiftBins,BinaryVideo>(index, &ShiftBins::processInputBinary);
    }
    else {
	return false;
    }

    return registerParameter(shift_) && registerParameter(enabled_) && Super::startup();
}

bool
ShiftBins::processInputVideo(const Video::Ref& msg)
{
    return process<Video>(msg);
}

bool
ShiftBins::processInputBinary(const BinaryVideo::Ref& msg)
{
    return process<BinaryVideo>(msg);
}

void
ShiftBins::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kShift, shift_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[ShiftBins::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    int shift = status[ShiftBins::kShift];
    return Algorithm::FormatInfoValue(QString("Shift %1 %2")
                                      .arg(shift < 0 ? "left" : "right")
                                      .arg(::abs(shift)));
}

// Factory function for the DLL that will create a new instance of the ShiftBins class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
ShiftBinsMake(Controller& controller, Logger::Log& log)
{
    return new ShiftBins(controller, log);
}
