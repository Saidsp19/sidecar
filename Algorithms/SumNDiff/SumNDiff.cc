#include "boost/bind.hpp" // for std::transform
#include <algorithm>      // for std::transform
#include <functional>     // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "SumNDiff.h"
#include "SumNDiff_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
SumNDiff::SumNDiff(Controller& controller, Logger::Log& log) :
    Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
    radius_(Parameter::PositiveIntValue::Make("radius", "Size of windows to sum and subtract", kDefaultRadius)),
    sum1_(), sum2_(), buffer_()
{
    radius_->connectChangedSignalTo(boost::bind(&SumNDiff::radiusChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the SumNDiff
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
SumNDiff::startup()
{
    registerProcessor<SumNDiff, Messages::Video>(&SumNDiff::processInput);
    return registerParameter(radius_) && registerParameter(enabled_) && Super::startup();
}

bool
SumNDiff::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

bool
SumNDiff::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    buffer_.push_back(msg);

    // Resize messages as needed
    //
    if (msg->size() > sum1_.size()) {
        sum1_.resize(msg->size());
        sum2_.resize(msg->size());
    }

    // Don't process anything until we have accumulated enough messages
    //
    if (buffer_.size() < size_t(2 * radius_->getValue())) { return true; }

    std::fill(sum1_.begin(), sum1_.end(), 0);
    std::fill(sum2_.begin(), sum2_.end(), 0);

    VideoMessageBuffer::iterator itr;
    VideoMessageBuffer::reverse_iterator ritr;
    size_t R = radius_->getValue();
    for (itr = buffer_.begin(), ritr = buffer_.rbegin(); itr != buffer_.begin() + R; itr++, ritr++) {
        std::transform(sum1_.begin(), sum1_.end(), (*itr)->begin(), sum1_.begin(),
                       std::plus<Messages::Video::DatumType>());
        std::transform(sum2_.begin(), sum2_.end(), (*ritr)->begin(), sum2_.begin(),
                       std::plus<Messages::Video::DatumType>());
    }

    // Create a new message to hold the output of what we do. Note that although we pass in the input message, the new
    // message does not contain any data.
    //
    Messages::Video::Ref out(Messages::Video::Make("SumNDiff::processInput", msg));

    // By default the new message has no elements. Either append them or resize.
    //
    out->resize(sum1_.size());

    // Fill the buffer with the difference between sum1 and sum2
    //
    std::transform(sum1_.begin(), sum1_.end(), sum2_.begin(), out->begin(), std::minus<Messages::Video::DatumType>());
    buffer_.pop_front();

    // Send out on the default output device, and return the result to our Controller. NOTE: for multichannel output,
    // one must give a channel index to the send() method. Use getOutputChannelIndex() to obtain the index for an
    // output channel with a given name.
    //
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
SumNDiff::radiusChanged(const Parameter::PositiveIntValue& parameter)
{
    buffer_.clear();
}

void
SumNDiff::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[SumNDiff::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the SumNDiff class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SumNDiffMake(Controller& controller, Logger::Log& log)
{
    return new SumNDiff(controller, log);
}
