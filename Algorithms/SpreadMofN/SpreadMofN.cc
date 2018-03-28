#include "boost/bind.hpp"

#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "SpreadMofN.h"
#include "SpreadMofN_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
SpreadMofN::SpreadMofN(Controller& controller, Logger::Log& log) :
    Super(controller, log, kDefaultEnabled, kDefaultCpiSpan),
    M_(Parameter::PositiveIntValue::Make("M", "Number of positive values to pass<br>", kDefaultM))
{
    cpiSpan_->connectChangedSignalTo(boost::bind(&SpreadMofN::cpiSpanChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the SpreadMofN
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
SpreadMofN::startup()
{
    return registerParameter(M_) && Super::startup();
}

bool
SpreadMofN::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

// This routine is responsible for taking a set of PRIs and determing if enough "hits" are present for each
// Doppler-range in the CPI to merit a "hit" for the entire Doppler-range bin.
//
bool
SpreadMofN::processCPI()
{
    static Logger::ProcLog log("processCPI", getLog());
    LOGINFO << std::endl;
    LOGDEBUG << "Process CPI with " << buffer_.size() << " msgs" << std::endl;

    size_t cpiSpan = cpiSpan_->getValue();
    int M = M_->getValue();

    if (buffer_.size() != cpiSpan) {
        LOGWARNING << "Kicking an incomplete CPI!  Expected " << cpiSpan << ", but received " << buffer_.size()
                   << std::endl;
        return true;
    }

    std::vector<Messages::BinaryVideo::Ref> msg_buffer_;
    for (Super::MessageQueue::iterator itr = buffer_.begin(); itr != buffer_.end(); itr++) {
        msg_buffer_.push_back(boost::dynamic_pointer_cast<Messages::BinaryVideo>(*itr));
    }

    size_t msg_size = msg_buffer_[0]->size();
    Messages::BinaryVideo::Ref out(Messages::BinaryVideo::Make(getName(), msg_buffer_[0]));
    Messages::BinaryVideo::Container& outputData(out->getData());
    outputData.resize(msg_buffer_[0]->size(), 0);

    // Count only Doppler bins in interval [2, cpiSpan - 1]
    //
    for (size_t i = 2; i < cpiSpan - 1; i++) {
        for (size_t j = 0; j < msg_size; j++) {
            if (msg_buffer_[i][j]) { ++outputData[j]; }
        }
    }

    // Check if enough hits were detected
    //
    for (size_t i = 0; i < msg_size; i++) { outputData[i] = (outputData[i] >= M); }

    msg_buffer_.clear();
    bool rc = send(out);

    return rc;
}

bool
SpreadMofN::cpiSpanChanged(const Parameter::PositiveIntValue& parameter)
{
    return true;
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[CPIAlgorithm::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the SpreadMofN class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
SpreadMofNMake(Controller& controller, Logger::Log& log)
{
    return new SpreadMofN(controller, log);
}
