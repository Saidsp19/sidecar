#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Algorithms/Utils.h"
#include "Logger/Log.h"
#include "Messages/TSPI.h"

#include "Alarmer.h"
#include "Alarmer_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Algorithms;

Alarmer::Alarmer(Controller& controller, Logger::Log& log) :
    Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled))
{
}

bool
Alarmer::startup()
{
    registerProcessor<Alarmer, Video>(&Alarmer::processInput);
    setAlarm(10); // set an alarm to go off every 10 seconds
    return registerParameter(enabled_) && Super::startup();
}

void
Alarmer::processAlarm()
{
    std::cerr << "Alarmer::my_handler" << std::endl;
}

bool
Alarmer::shutdown()
{
    return Super::shutdown();
}

bool
Alarmer::processInput(const Messages::Video::Ref& msg)
{
    return send(msg);
}

void
Alarmer::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    QString value(!status[Alarmer::kEnabled] ? "Disabled" : "Enabled");
    return Utils::formatInfoValue(value);
}

// Factory function for the DLL that will create a new instance of the ABTracker class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
AlarmerMake(Controller& controller, Logger::Log& log)
{
    return new Alarmer(controller, log);
}
