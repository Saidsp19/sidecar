#include <functional>

#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Video.h"

#include "DynamicThreshold.h"
#include "DynamicThreshold_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

static const char* kOperatorNames[] = {
    " < ",
    " <= ",
    " == ",
    " >= ",
    " > ",
};

const char* const*
DynamicThreshold::OperatorEnumTraits::GetEnumNames()
{
    return kOperatorNames;
}

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
DynamicThreshold::DynamicThreshold(Controller& controller, Logger::Log& log)
    : Super(controller, log, kDefaultEnabled, kDefaultMaxBufferSize),
      passPercentage_(1000, 0.0),
      operator_(OperatorParameter::Make("operator", "Operator", Operator(kDefaultOperator)))
{
    ;
}

ChannelBuffer*
DynamicThreshold::makeChannelBuffer(int index, const std::string& name, size_t maxBufferSize)
{
    static Logger::ProcLog log("makeChannelBuffer", getLog());
    LOGINFO << std::endl;

    if (name == "samples") {
	samples_ = new TChannelBuffer<Video>(*this, index, maxBufferSize);
	return samples_;
    }
    else if (name == "thresholds") {
	thresholds_ = new TChannelBuffer<Video>(*this, index, maxBufferSize);
	return thresholds_;
    }
    else {
	LOGFATAL << "invalid channel name - " << name << std::endl;
    }

    return 0;
}

bool
DynamicThreshold::startup()
{
    return registerParameter(operator_) && Super::startup();
}

namespace {

/** Template functor that keeps track of the number of times a conditional expression evaluates true. The sole
    template argument is a binary_function-based template class that defines the conditional expression to
    execute via its operator() method (eg. std::less)
*/
template <template <typename> class T>
struct TProc : public T<Video::DatumType>
{
    using Super = T<Video::DatumType>;
    using ArgType = typename Super::first_argument_type;
    size_t& c_;
    TProc(size_t& c) : c_(c) {}

    /** Count the number of times the given argument is true.
        
        \param v the value to check

        \return the given value
    */
    bool countTrue(bool v) { if (v) ++c_; return v; }

    /** Compare a sample value against a threshold value using an STL binary functor to do the comparison.
        Records if the functor returns true.

        \param sample the sample value to compare

        \param threshold the threshold value to compare

        \return result of the comparison (true or false)
    */
    bool operator()(ArgType sample, ArgType threshold) { return countTrue(Super::operator()(sample, threshold)); }
};

// Define instantiations of the TProc template for used conditional expressions.
//
using LessThan = TProc<std::less>;
using LessThanEqualTo = TProc<std::less_equal>;
using EqualTo = TProc<std::equal_to>;
using GreaterThanEqualTo = TProc<std::greater_equal>;
using GreaterThan = TProc<std::greater>;
}

bool
DynamicThreshold::processChannels()
{
    static Logger::ProcLog log("processChannels", getLog());
    LOGINFO << std::endl;

    Video::Ref samples(samples_->popFront());
    Video::Ref thresholds(thresholds_->popFront());
    if (thresholds->size() < samples->size()) {
	thresholds->resize(samples->size(), 0);
    }

    // Create our output message. Use std::transform to visit the sample and threshold values, calling the
    // appropriate functor for each and adding the binary result to the output message.
    //
    BinaryVideo::Ref out(BinaryVideo::Make("DynamicThreshold", samples));
    out->resize(samples->size());

    size_t passed = 0;
    switch (operator_->getValue()) {
    case kLessThan:
	std::transform(samples->begin(), samples->end(), thresholds->begin(),
                       out->begin(), LessThan(passed));
	break;
    case kLessThanEqualTo:
	std::transform(samples->begin(), samples->end(), thresholds->begin(),
                       out->begin(), LessThanEqualTo(passed));
	break;
    case kEqualTo:
	std::transform(samples->begin(), samples->end(), thresholds->begin(),
                       out->begin(), EqualTo(passed));
	break;
    case kGreaterThanEqualTo:
	std::transform(samples->begin(), samples->end(), thresholds->begin(),
                       out->begin(), GreaterThanEqualTo(passed));
	break;
    case kGreaterThan:
	std::transform(samples->begin(), samples->end(), thresholds->begin(),
                       out->begin(), GreaterThan(passed));
	break;
    default:
	LOGERROR << "invalid operation: " << operator_->getValue()
		 << std::endl;
	return false;
    }

    // Update our stats based on how many samples passed their threshold.
    //
    passPercentage_.addValue(double(passed) / out->size());
    bool rc = send(out);

    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
DynamicThreshold::setInfoSlots(IO::StatusBase& status)
{
    Super::setInfoSlots(status);
    status.setSlot(kOperator, operator_->getValue());
    status.setSlot(kPassPercentage, passPercentage_.getAverageValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;

    if (! status[ManyInAlgorithm::kEnabled])
	return Algorithm::FormatInfoValue(ManyInAlgorithm::GetFormattedStats(status));

    double passPercentage = status[DynamicThreshold::kPassPercentage];
    passPercentage *= 100.0;
    return Algorithm::FormatInfoValue(ManyInAlgorithm::GetFormattedStats(status) +
                                      QString("Comparision: %1  Passed: %2%")
                                      .arg(kOperatorNames[int(status[DynamicThreshold::kOperator])])
                                      .arg(passPercentage));
}

// Factory function for the DLL that will create a new instance of the DynamicThreshold class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
DynamicThresholdMake(Controller& controller, Logger::Log& log)
{
    return new DynamicThreshold(controller, log);
}
