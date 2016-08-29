#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "Clamp.h"
#include "Clamp_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

Clamp::Clamp(Controller& controller, Logger::Log& log)
    : Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      min_(Parameter::IntValue::Make("min", "Min Value", kDefaultMin)),
      max_(Parameter::IntValue::Make("max", "Max Value", kDefaultMax))
{
    ;
}

bool
Clamp::startup()
{
    registerProcessor<Clamp,Messages::Video>(&Clamp::processInput);
    bool ok = true;
    ok = ok && registerParameter(min_) && registerParameter(max_) && registerParameter(enabled_);
    return ok && Super::startup();
}

void
Clamp::setRange(int min, int max)
{
    min_->setValue(min);
    max_->setValue(max);
}

void
Clamp::setMinValue(int value)
{
    min_->setValue(value);
}

void
Clamp::setMaxValue(int value)
{
    max_->setValue(value);
}

bool
Clamp::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    // If not enabled, simply pass message.
    //
    if (! enabled_->getValue()) {
	return send(msg);
    }

    // Create a new message to hold the output of what we do. Note that although we pass in the input message,
    // the new message does not contain any data.
    //
    Messages::Video::Ref out(Messages::Video::Make("Clamp", msg));

    // Use the STL transform function to convert values from the input message into clamped values placed in the
    // output message. The std::back_inserter method creates an output iterator that uses push_back() when
    // inserting values.
    //
    using DatumType = Messages::Video::DatumType;
    DatumType minValue = min_->getValue();
    DatumType maxValue = max_->getValue();
    std::transform(msg->begin(), msg->end(), std::back_inserter(out->getData()),
                   [minValue, maxValue](auto value) {
                       return std::max(minValue, std::min(maxValue, value));
                   });

    // Send out on the default output device, and return the result to our Controller. NOTE: for multichannel
    // output, one must give a channel index to the send() method. Use getOutputChannelIndex() to obtain the
    // index for an output channel with a given name.
    //
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
Clamp::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kMin, min_->getValue());
    status.setSlot(kMax, max_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[Clamp::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return Algorithm::FormatInfoValue(QString("Min: %1  Max: %2").arg(int(status[Clamp::kMin]))
                                      .arg(int(status[Clamp::kMax])));
}

// Factory function for the DLL that will create a new instance of the Clamp class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
ClampMake(Controller& controller, Logger::Log& log)
{
    return new Clamp(controller, log);
}
