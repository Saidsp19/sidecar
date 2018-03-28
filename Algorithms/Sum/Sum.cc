#include "boost/bind.hpp"

#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "Sum.h"
#include "Sum_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
Sum::Sum(Controller& controller, Logger::Log& log) :
    Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
    bufferSize_(Parameter::PositiveIntValue::Make("bufferSize", "Number of PRIs to sum over", kDefaultBufferSize)),
    sums_(), buffer_()
{
    bufferSize_->connectChangedSignalTo(boost::bind(&Sum::bufferSizeChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the Sum class.
// Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as shown
// below.
//
bool
Sum::startup()
{
    registerProcessor<Sum, Messages::Video>(&Sum::processInput);
    return registerParameter(bufferSize_) && registerParameter(enabled_) && Super::startup();
}

bool
Sum::shutdown()
{
    // Release memory and other resources here.
    //
    buffer_.clear();
    sums_.clear();
    return Super::shutdown();
}

bool
Sum::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    // Resize sum vector if necessary
    //
    if (msg->size() > sums_.size()) { sums_.resize(msg->size(), 0); }

    buffer_.push_back(msg);

    // Sum the values
    //
    std::transform(sums_.begin(), sums_.end(), msg->begin(), sums_.begin(), std::plus<Messages::Video::DatumType>());

    // Create a new message to hold the output of what we do. For this algorithm, the message of relevance is
    // the newest message.
    //
    Messages::Video::Ref out(Messages::Video::Make("Sum::processInput", msg));

    // Put the contents of the summation vector into the output message.
    //
    std::vector<Messages::Video::DatumType>::iterator itr;
    for (itr = sums_.begin(); itr != sums_.end(); itr++) { out->push_back(*itr); }

    // If necessary, trim buffer and remove contributions of old messages.
    //
    while (buffer_.size() >= size_t(bufferSize_->getValue())) {
        std::transform(sums_.begin(), sums_.end(), buffer_.front()->begin(), sums_.begin(),
                       std::minus<Messages::Video::DatumType>());
        buffer_.pop_front();
    }

    // Send out on the default output device, and return the result to our Controller. NOTE: for multichannel output,
    // one must give a channel index to the send() method. Use getOutputChannelIndex() to obtain the index for an
    // output channel with a given name.
    //
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
Sum::bufferSizeChanged(const Parameter::PositiveIntValue& parameter)
{
    sums_.clear();
    buffer_.clear();
}

void
Sum::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[Sum::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the Sum class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SumMake(Controller& controller, Logger::Log& log)
{
    return new Sum(controller, log);
}
