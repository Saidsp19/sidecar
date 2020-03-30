#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Logger/Log.h"

#include "Inverter.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

Inverter::Inverter(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), min_(Parameter::IntValue::Make("min", "Min Sample", -8192)),
    max_(Parameter::IntValue::Make("max", "Max Sample", 8192))
{
    ;
}

bool
Inverter::startup()
{
    registerProcessor<Inverter, Messages::Video>(&Inverter::process);
    return registerParameter(min_) && registerParameter(max_) && Algorithm::startup();
}

Inverter::DatumType
Inverter::invert(DatumType value) const
{
    return (max_->getValue() - value) + min_->getValue();
}

bool
Inverter::process(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());

    // We use the STL transform method to do looping for us. We tell it the first data point to process, a
    // marker indicating the end of the data, and a function object to invoke to do the inversion. We use two
    // STL methods to generate a function object: std::mem_fun - wrapper around a member function/method
    // std::bind1st - a value binder that assigns a value to the 1st value of a method. For member functions,
    // the first argument is always an object of the type that has the method
    //
    std::transform(msg->begin(), msg->end(), msg->begin(), [this](auto const& value){ return invert(value); });

    // Transformation is done -- send out on the default output device.
    //
    bool rc = send(msg);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

extern "C" ACE_Svc_Export Algorithm*
InverterMake(Controller& controller, Logger::Log& log)
{
    return new Inverter(controller, log);
}
