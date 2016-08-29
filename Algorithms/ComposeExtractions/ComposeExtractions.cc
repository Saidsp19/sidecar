#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/Extraction.h"
#include "Utils/Utils.h"

#include "ComposeExtractions.h"
#include "ComposeExtractions_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Algorithms;

ComposeExtractions::ComposeExtractions(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make(
                   "enabled", "Enabled", kDefaultEnabled))
{

}

bool
ComposeExtractions::startup()
{

    setAlarm(10);
    timerCount_ = 0;
    //    registerProcessor<ComposeExtractions,Track>(&ComposeExtractions::processInput);
    return registerParameter(enabled_) &&
        Super::startup();
}


void 
ComposeExtractions::processAlarm()
{
  
    static Logger::ProcLog log("my_handler", getLog());

    Messages::Extractions::Ref msg = Messages::Extractions::Make("ComposeExtractions", Messages::Header::Ref());

    double range = 0;
    double az = 45;

    if (timerCount_ == 0)
        range = 100;
 
    if (timerCount_ == 1)
        range = 101;
  
    if (timerCount_ == 2)
        range = 102;

    if (timerCount_ == 3)
        range = 103;
 
    if (timerCount_ == 4)
        range = 104;
  
    if (timerCount_ == 5)
        range = 105;


    if (timerCount_ < 6){
        Time::TimeStamp tstamp = Time::TimeStamp::Now();

        LOGERROR << "sending extraction with timestamp " << tstamp.asDouble() << std::endl;

        msg->push_back(Messages::Extraction(tstamp,
                                            range,
                                            az * M_PI / 180.0,
                                            0.0));

    
        send(msg);
    }

    timerCount_++;
  
}


bool
ComposeExtractions::shutdown()
{
    return Super::shutdown();
}


/*bool
  ComposeExtractions::processInput(const Messages::Track::Ref& msg)
  {
  fprintf(stderr, "ComposeExtractions::processInput\n");
  

   

  return send(msg);
  }*/


void
ComposeExtractions::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[ComposeExtractions::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
ComposeExtractionsMake(Controller& controller, Logger::Log& log)
{
    return new ComposeExtractions(controller, log);
}
