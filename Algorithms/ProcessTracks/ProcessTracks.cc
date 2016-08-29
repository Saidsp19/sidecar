#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/TSPI.h"
#include "Messages/Track.h"

#include "ProcessTracks.h"
#include "ProcessTracks_defaults.h"
#include "Utils/Utils.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Algorithms;

ProcessTracks::ProcessTracks(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make(
                   "enabled", "Enabled", kDefaultEnabled))
{
  
}

bool
ProcessTracks::startup()
{
    registerProcessor<ProcessTracks,Track>(&ProcessTracks::processInput);
    return registerParameter(enabled_) &&
        Super::startup();
}



bool
ProcessTracks::shutdown()
{
    return Super::shutdown();
}


bool
ProcessTracks::processInput(const Messages::Track::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    LOGERROR << msg->printHeader(std::cerr) << std::endl;
    LOGERROR << msg->printData(std::cerr) << std::endl;

  
    return send(msg);
}


void
ProcessTracks::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[ProcessTracks::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the ABTracker class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
ProcessTracksMake(Controller& controller, Logger::Log& log)
{
    return new ProcessTracks(controller, log);
}
