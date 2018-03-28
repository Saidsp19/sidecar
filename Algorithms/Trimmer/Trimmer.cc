#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Utils.h"
#include "Logger/Log.h"

#include "Trimmer.h"
#include "Trimmer_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

Trimmer::Trimmer(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log),
    firstSample_(Parameter::NonNegativeIntValue::Make("firstSample", "First sample to keep", kDefaultFirstSample)),
    maxSampleCount_(
        Parameter::NonNegativeIntValue::Make("maxSampleCount", "Max Sample Count (0 for all)", kDefaultMaxSampleCount)),
    complexSamples_(Parameter::BoolValue::Make("complexSamples", "Complex Samples", kDefaultComplexSamples)),
    enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled))
{
    endParameterChanges();
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// Trimmer class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
Trimmer::startup()
{
    registerProcessor<Trimmer, Messages::Video>(&Trimmer::processInput);
    return registerParameter(firstSample_) && registerParameter(maxSampleCount_) &&
           registerParameter(complexSamples_) && registerParameter(enabled_) && Algorithm::startup();
}

void
Trimmer::setFirstSample(size_t value)
{
    firstSample_->setValue(value);
    endParameterChanges();
}

void
Trimmer::setMaxSampleCount(size_t value)
{
    maxSampleCount_->setValue(value);
    endParameterChanges();
}

void
Trimmer::setComplexSamples(bool value)
{
    complexSamples_->setValue(value);
    endParameterChanges();
}

void
Trimmer::endParameterChanges()
{
    // Cache parameter values.
    //
    isComplex_ = complexSamples_->getValue();

    // Fetch the index of the first sample to use.
    //
    firstIndex_ = firstSample_->getValue();
    if (isComplex_) firstIndex_ *= 2;

    // Fetch the maximum sample count for the output message. Convert into the index of the first element NOT
    // copied.
    //
    lastIndex_ = maxSampleCount_->getValue();
    if (isComplex_) lastIndex_ *= 2;
    lastIndex_ += firstIndex_;
}

bool
Trimmer::processInput(const Messages::Video::Ref& msg)
{
    Messages::Video::Ref out(Messages::Video::Make(getName(), msg));

    // When disabled, just copy the data over to the output message and be done with it.
    //
    if (!enabled_->getValue()) {
        out->getData() = msg->getData();
        return send(out);
    }

    // Establish range of samples to copy over. Note that we do not validate the start index, since it happens
    // automatically by comparison with the limit value, which is validated.
    //
    size_t index = firstIndex_;
    size_t limit = lastIndex_;
    if (limit == index || limit > msg->size()) limit = msg->size();

    // Copy the samples. Q: would std::copy be faster here?
    //
    while (index < limit) out->push_back(msg[index++]);

    // And send
    //
    return send(out);
}

void
Trimmer::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kFirstSample, firstSample_->getValue());
    status.setSlot(kMaxSampleCount, maxSampleCount_->getValue());
    status.setSlot(kComplexSamples, complexSamples_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;

    QString value;
    if (!status[Trimmer::kEnabled]) {
        value = "Disabled";
    } else {
        value = QString("First: %1 Limit: %2  Complex: %3")
                    .arg(static_cast<int>(status[Trimmer::kFirstSample]))
                    .arg(static_cast<int>(status[Trimmer::kMaxSampleCount]))
                    .arg("NY"[static_cast<bool>(status[Trimmer::kComplexSamples])]);
    }

    return Algorithm::FormatInfoValue(value);
}

// Factory function for the DLL that will create a new instance of the Trimmer class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
TrimmerMake(Controller& controller, Logger::Log& log)
{
    return new Trimmer(controller, log);
}
