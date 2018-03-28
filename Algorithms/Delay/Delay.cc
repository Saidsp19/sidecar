#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "Delay.h"
#include "Delay_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
Delay::Delay(Controller& controller, Logger::Log& log) :
    Super(controller, log), buffer_(), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
    delay_(Parameter::PositiveIntValue::Make("delay", "# of messages to delay by", kDefaultDelay))
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the Delay class.
// Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as shown
// below.
//
bool
Delay::startup()
{
    const IO::Channel& channel(getController().getInputChannel(0));

    if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        registerProcessor<Delay, Messages::Video>(&Delay::processInputVideo);
    } else if (channel.getTypeKey() == Messages::BinaryVideo::GetMetaTypeInfo().getKey()) {
        registerProcessor<Delay, Messages::BinaryVideo>(&Delay::processInputBinary);
    } else {
        return false;
    }

    return registerParameter(delay_) && registerParameter(enabled_) && Super::startup();
}

bool
Delay::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
Delay::processInputVideo(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInputVideo", getLog());

    // Perform delay operation and check for processing conditions
    //
    buffer_.push_back(msg);
    if (buffer_.size() < size_t(delay_->getValue())) { return true; }

    // Get the delayed message
    //
    Messages::Video::Ref ref = boost::dynamic_pointer_cast<Messages::Video>(buffer_.front());

    // Create output message's header based on newly arrived message
    //
    Messages::Video::Ref out = Messages::Video::Make(getName(), msg);

    // Copy delayed data into new message
    //
    out->getData() = ref->getData();

    // Pop off old message
    //
    buffer_.pop_front();

    bool rc = send(out);

    return rc;
}

bool
Delay::processInputBinary(const Messages::BinaryVideo::Ref& msg)
{
    static Logger::ProcLog log("processInputBinary", getLog());

    // Perform delay operation and check for processing conditions
    //
    buffer_.push_back(msg);
    if (buffer_.size() < size_t(delay_->getValue())) { return true; }

    // Get the delayed message
    //
    Messages::BinaryVideo::Ref ref = boost::dynamic_pointer_cast<Messages::BinaryVideo>(buffer_.front());

    // Create output message's header based on newly arrived message
    //
    Messages::BinaryVideo::Ref out = Messages::BinaryVideo::Make(getName(), msg);

    // Copy delayed data into new message
    //
    out->getData() = ref->getData();

    // Pop off old message
    //
    buffer_.pop_front();

    bool rc = send(out);

    return rc;
}

void
Delay::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[Delay::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the Delay class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
DelayMake(Controller& controller, Logger::Log& log)
{
    return new Delay(controller, log);
}
