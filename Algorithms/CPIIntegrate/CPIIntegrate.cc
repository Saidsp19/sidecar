#include "boost/bind.hpp"

#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "CPIIntegrate.h"
#include "CPIIntegrate_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
CPIIntegrate::CPIIntegrate(Controller& controller, Logger::Log& log) :
    Super(controller, log, kDefaultEnabled, kDefaultCpiSpan),
    numCPIs_(Parameter::PositiveIntValue::Make("numCPIs", "Number of CPIs to average<br>", kDefaultNumCPIs)), cpis_(),
    vals_()
{
    cpiSpan_->connectChangedSignalTo(boost::bind(&CPIIntegrate::cpiSpanChanged, this, _1));
    numCPIs_->connectChangedSignalTo(boost::bind(&CPIIntegrate::numCPIsChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the CPIIntegrate
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
CPIIntegrate::startup()
{
    vals_.resize(cpiSpan_->getValue());
    std::vector<std::vector<float>>::iterator itr;
    for (itr = vals_.begin(); itr != vals_.end(); itr++) { (*itr).resize(1000, 0.0); }

    return registerParameter(numCPIs_) && Super::startup();
}

// Hook for handling when the user changes the CPI span size
//
bool
CPIIntegrate::cpiSpanChanged(const Parameter::PositiveIntValue& parameter)
{
    reset();
    return true;
}

// Hook for handling when the user changes the number of CPIs to integrate over
//
void
CPIIntegrate::numCPIsChanged(const Parameter::PositiveIntValue& parameter)
{
    reset();
}

// Routine that rebuilds the buffers for integration
//
bool
CPIIntegrate::reset()
{
    size_t cpiSpan = cpiSpan_->getValue();
    size_t msg_size = vals_[0].size();

    // Reset running averages
    //
    vals_.clear();
    vals_.resize(cpiSpan);
    for (size_t i = 0; i < cpiSpan; i++) { vals_[i].resize(msg_size, 0.0); }

    // Reset message buffers
    //
    while (!cpis_.empty()) {
        MessageQueue* ptr = cpis_.back();
        cpis_.pop_back();
        delete ptr;
    }

    cpis_.clear();
    buffer_.clear();

    return true;
}

bool
CPIIntegrate::shutdown()
{
    // Release memory and other resources here.
    //
    reset();
    vals_.clear();

    return Super::shutdown();
}

// This routine is responsible for taking a set of PRIs which constitute a CPI and performing the appropriate
// operation.
//
bool
CPIIntegrate::processCPI()
{
    static Logger::ProcLog log("processCPI", getLog());
    LOGINFO << std::endl;
    LOGDEBUG << "Process CPI with " << buffer_.size() << " msgs" << std::endl;

    bool rc = true;

    size_t numCPIs = numCPIs_->getValue();
    size_t cpiSpan = cpiSpan_->getValue();

    int last_row = -1;
    Super::MessageQueue::iterator itr;
    size_t cnt, row;

    if (buffer_.size() != cpiSpan) {
        LOGWARNING << "Kicking an incomplete CPI!  Expected " << cpiSpan << ", but received " << buffer_.size()
                   << std::endl;
        return true;
    }

    size_t msg_size = maxMsgSize();

    // Resize the vals_ buffer if necessary adding default value of 0
    //
    if (msg_size > vals_[0].size()) {
        for (size_t i = 0; i < cpiSpan; i++) { vals_[i].resize(msg_size, 0.0); }
    }

    MessageQueue* cpi = new MessageQueue;
    uint32_t startingSequenceNumber_ = buffer_[0]->getRIUInfo().sequenceCounter;

    // Push this cpi into the buffer of CPIs
    //
    for (itr = buffer_.begin(), cnt = 0; itr != buffer_.end() && cnt < cpiSpan; itr++, cnt++) {
        Messages::Video::Ref ref = boost::dynamic_pointer_cast<Messages::Video>(*itr);
        row = ref->getRIUInfo().sequenceCounter - startingSequenceNumber_;
        // pad any message locations whose PRIs were dropped
        //
        for (size_t row_index = last_row + 1; row_index < row; row_index++) {
            LOGDEBUG << "Padding CPI(" << row_index << ") with zeroes" << std::endl;
            Messages::Video::Ref msg(Messages::Video::Make(getName(), ref));
            size_t N = vals_[0].size();
            for (size_t i = 0; i < N; i++) { msg->push_back(0); }
            cpi->push_back(msg);
        }

        cpi->push_back(ref);
        last_row = row++;
    }

    // Add contribution of new CPI to running average
    //
    for (size_t i = 0; i < cpiSpan; i++) {
        Messages::Video::Ref ref = boost::dynamic_pointer_cast<Messages::Video>(cpi->front());
        size_t n = ref->size();
        // Add this message's values to the running average
        for (size_t j = 0; j < n; j++) { vals_[i][j] += ref[j]; }
        // remove the message from the front of the queue and push it to the back of the queue
        //
        cpi->pop_front();
        cpi->push_back(ref);
    }

    cpis_.push_front(cpi);

    // Make sure we have enough CPI blocks to compute the averages
    //
    if (cpis_.size() < size_t(numCPIs + 1)) { return rc; }

    // Remove contribution of oldest CPI from running average
    //
    cpi = cpis_.back();
    cpis_.pop_back();

    for (size_t i = 0; i < cpiSpan; i++) {
        Messages::Video::Ref ref = boost::dynamic_pointer_cast<Messages::Video>(cpi->front());
        size_t n = ref->size();
        for (size_t j = 0; j < n; j++) vals_[i][j] -= ref[j];
    }
    // De-allocate memory used to hold this CPI
    //
    delete cpi;

    // Results are in reference to the middle CPI, ie average values are composed of CPIs that come before
    // and after this CPI.
    //
    size_t middle = (numCPIs / 2) + 1;

    // Compute the averages and send the values out
    //
    for (size_t i = 0; i < cpiSpan; i++) {
        Messages::Video::Ref parent = boost::dynamic_pointer_cast<Messages::Video>((*(cpis_[middle]))[i]);
        Messages::Video::Ref out(Messages::Video::Make(getName(), parent));
        Messages::Video::Container& outputData(out->getData());
        size_t N = vals_[i].size();

        for (size_t j = 0; j < N; j++) {
            outputData.push_back(Messages::Video::DatumType(::rint(vals_[i][j] / numCPIs)));
        }

        rc = rc && send(out);
    }

    return rc;
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[CPIAlgorithm::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the CPIIntegrate class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
CPIIntegrateMake(Controller& controller, Logger::Log& log)
{
    return new CPIIntegrate(controller, log);
}
