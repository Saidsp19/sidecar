#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "IO/Channel.h"
#include "Logger/Log.h"

#include "FanOut.h"
#include "FanOut_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
FanOut::FanOut(Controller& controller, Logger::Log& log) : Super(controller, log)
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the FanOut
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
FanOut::startup()
{
    const IO::Channel& channel(getController().getInputChannel(0));
    if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        registerProcessor<FanOut, Messages::Video>(&FanOut::processInputVideo);
    } else if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        registerProcessor<FanOut, Messages::BinaryVideo>(&FanOut::processInputBinary);
    } else {
        return false;
    }

    return Super::startup();
}

bool
FanOut::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
FanOut::processInputVideo(const Messages::Video::Ref& msg)
{
    bool rc = true;
    for (size_t index = 0; index < getController().getNumOutputChannels(); ++index) {
        if (!send(msg, index)) rc = false;
    }

    return rc;
}

bool
FanOut::processInputBinary(const Messages::BinaryVideo::Ref& msg)
{
    bool rc = true;
    for (size_t index = 0; index < getController().getNumOutputChannels(); ++index) {
        if (!send(msg, index)) rc = false;
    }

    return rc;
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    return NULL;
}

// Factory function for the DLL that will create a new instance of the FanOut class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
FanOutMake(Controller& controller, Logger::Log& log)
{
    return new FanOut(controller, log);
}
