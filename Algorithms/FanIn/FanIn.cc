#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "FanIn.h"
#include "FanIn_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
FanIn::FanIn(Controller& controller, Logger::Log& log)
    : Super(controller, log)
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the FanIn class.
// Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as shown
// below.
//
bool
FanIn::startup()
{
    const IO::Channel& channel(getController().getInputChannel(0));
    if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
	for (size_t index = 0; index < getController().getNumInputChannels(); ++index) {
	    registerProcessor<FanIn,Messages::Video>(index, &FanIn::processInputVideo);
	}
    }
    else if (channel.getTypeKey() == Messages::BinaryVideo::GetMetaTypeInfo().getKey()) {
	for (size_t index = 0; index < getController().getNumInputChannels(); ++index) {
	    registerProcessor<FanIn,Messages::BinaryVideo>(index, &FanIn::processInputBinary);
	}
    }
    else {
	return false;
    }

    return Super::startup();
}

bool
FanIn::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
FanIn::processInputVideo(const Messages::Video::Ref& msg)
{
    return send(msg);
}

bool
FanIn::processInputBinary(const Messages::BinaryVideo::Ref& msg)
{
    return send(msg);
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    return NULL;
}

// Factory function for the DLL that will create a new instance of the FanIn class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
FanInMake(Controller& controller, Logger::Log& log)
{
    return new FanIn(controller, log);
}
