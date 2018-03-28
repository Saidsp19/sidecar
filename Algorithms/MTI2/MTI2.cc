#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Utils/VsipVector.h"

#include "MTI2.h"
#include "MTI2_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

using VsipVideoVector = VsipVector<Messages::Video>;

MTI2::MTI2(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), past_(2), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled))
{
    ;
}

bool
MTI2::startup()
{
    registerProcessor<MTI2, Messages::Video>(&MTI2::process);
    return registerParameter(enabled_) && Algorithm::startup();
}

bool
MTI2::reset()
{
    past_.clear();
    return true;
}

struct CalcMTI2 {
    Messages::Video::DatumType operator()(Messages::Video::DatumType in0, Messages::Video::DatumType in1) const
    {
        static float kScale = M_SQRT1_2;
        return Messages::Video::DatumType(::rintf((in0 - in1) * kScale));
    }
};

bool
MTI2::process(const Messages::Video::Ref& input)
{
    static Logger::ProcLog log("process", getLog());
    LOGDEBUG << std::endl;

    if (!enabled_->getValue()) return send(input);

    // Add message to the input buffer. Continue on only if we have 2 messages.
    //
    past_.add(input);
    if (!past_.full()) return true;

    // Create output message, and resize to the current max message size.
    //
    Messages::Video::Ref output(Messages::Video::Make(getName(), input));
    output->reserve(past_.getMaxMsgSize());

    std::transform(past_[0]->begin(), past_[0]->end(), past_[1]->begin(), std::back_inserter<>(output->getData()),
                   CalcMTI2());

    bool rc = send(output);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

extern "C" ACE_Svc_Export Algorithm*
MTI2Make(Controller& controller, Logger::Log& log)
{
    return new MTI2(controller, log);
}
