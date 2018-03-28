#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "Recorder.h"
#include "Recorder_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

Recorder::Recorder(Controller& controller, Logger::Log& log) : Super(controller, log)
{
    ;
}

bool
Recorder::startup()
{
    const IO::Channel& channel(getController().getInputChannel(0));
    if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        registerProcessor<Recorder, Messages::Video>(&Recorder::processInputVideo);
    } else if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        registerProcessor<Recorder, Messages::BinaryVideo>(&Recorder::processInputBinary);
    } else {
        return false;
    }

    return Super::startup();
}

bool
Recorder::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
Recorder::processInputVideo(const Messages::Video::Ref& msg)
{
    return send(msg);
}

bool
Recorder::processInputBinary(const Messages::BinaryVideo::Ref& msg)
{
    return send(msg);
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    return NULL;
}

// Factory function for the DLL that will create a new instance of the Recorder class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
RecorderMake(Controller& controller, Logger::Log& log)
{
    return new Recorder(controller, log);
}
