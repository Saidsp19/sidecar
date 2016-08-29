#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Algorithms/Utils.h"
#include "Logger/Log.h"

#include "WindowedMinMax.h"
#include "WindowedMinMax_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kValueGeneratorNames[] = {
    "Average",
    "Min",
    "Max",
    "MaxMin",
    "MaxMax",
};

const char* const*
WindowedMinMax::ValueGeneratorEnumTraits::GetEnumNames()
{
    return kValueGeneratorNames;
}

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
WindowedMinMax::WindowedMinMax(Controller& controller, Logger::Log& log)
    : Super(controller, log),
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
      windowSize_(Parameter::PositiveIntValue::Make("windowSize", "Window Size", kDefaultWindowSize)),
      windowCount_(Parameter::PositiveIntValue::Make("windowCount", "Window Count", kDefaultWindowCount)),
      windowOffset_(Parameter::NonNegativeIntValue::Make("windowOffset", "Window Offset", kDefaultWindowOffset)),
      valueGenerator_(ValueGeneratorParameter::Make("valueGenerator", "Value Generator Function",
                                                    ValueGenerator(kDefaultValueGenerator)))
{
    valueGeneratorChanged(*valueGenerator_);
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// WindowedMinMax class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
WindowedMinMax::startup()
{
    registerProcessor<WindowedMinMax,Messages::Video>(
        &WindowedMinMax::processInput);
    return registerParameter(enabled_) &&
	registerParameter(windowSize_) &&
	registerParameter(windowCount_) &&
	registerParameter(windowOffset_) &&
	registerParameter(valueGenerator_) &&
	Super::startup();
}

bool
WindowedMinMax::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    // Create a new message to hold the output of what we do. Note that although we pass in the input message,
    // the new message does not contain any data.
    //
    Messages::Video::Ref out(
        Messages::Video::Make("WindowedMinMax::processInput", msg));

    int windowCount = windowCount_->getValue();
    int windowSize = windowSize_->getValue();
    int windowOffset = windowOffset_->getValue();

    // Reset the running window sum values
    //
    windowSums_.clear();
    windowIndices_.clear();

    // Generate the sliding window indices and sums for those below the first sample point at index 0. Since
    // there is nothing below the first sample point, these sums are zero.
    //
    int index = - (windowCount * windowSize + windowOffset);
    for (int count = 0; count < windowCount; ++count) {
	int sum = 0;
	windowSums_.push_back(sum);
	windowIndices_.push_back(index);
	index += windowSize;
    }

    // Generate the sliding window indices and sums for those above the first sample point at index 0.
    //
    index = windowOffset;
    for (int count = 0; count < windowCount; ++count) {
	windowIndices_.push_back(index);

	// Calculate the sum of the values in one window.
	//
	int sum = 0;
	for (int sample = 0; sample < windowSize; ++sample)
	    sum += msg[index++];

	windowSums_.push_back(sum);
    }

    // Now that we are primed, process the message, shifting all of the windows by one sample each iteration.
    //
    int msgSize = msg->size();
    for (int sample = 0; sample < msgSize; ++sample) {

	// Calculate the output value derived from the sum windows.
	//
	out->push_back((this->*valueGeneratorProc_)());

	// Shift the windows.
	//
	for (size_t count = 0; count < windowIndices_.size(); ++count) {

	    // Update window sums by removing the oldest element and adding the new one. Always validate indices
	    // before using to protect against ends of the input message.
	    //
	    int value = windowSums_[count];
	    index = windowIndices_[count];
	    if (index >= 0 && index < msgSize)
		value -= msg[index];

	    index += windowSize;
	    if (index >= 0 && index < msgSize)
		value += msg[index];

	    windowSums_[count] = value;

	    // Shift indices up one.
	    //
	    windowIndices_[count] = index - windowSize + 1;
	}
    }

    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

int
WindowedMinMax::getAverage() const
{
    int sum = 0;
    size_t size = windowSums_.size();
    for (size_t index = 0; index < size; ++index)
	sum += windowSums_[index];
    return ::rint(double(sum) / size);
}

int
WindowedMinMax::getMin() const
{
    int found = windowSums_[0];
    size_t size = windowSums_.size();
    for (size_t index = 1; index < size; ++index)
	if (windowSums_[index] < found)
	    found = windowSums_[index];
    return found;
}

int
WindowedMinMax::getMax() const
{
    int found = windowSums_[0];
    size_t size = windowSums_.size();
    for (size_t index = 1; index < size; ++index)
	if (windowSums_[index] > found)
	    found = windowSums_[index];
    return found;
}

int
WindowedMinMax::getMaxMin() const
{
    size_t size = windowCount_->getValue();
    return std::max(*std::min_element(windowSums_.begin(),
                                      windowSums_.begin() + size),
                    *std::min_element(windowSums_.begin() + size,
                                      windowSums_.end())) +
	std::min(*std::max_element(windowSums_.begin(),
                                   windowSums_.begin() + size),
                 *std::max_element(windowSums_.begin() + size,
                                   windowSums_.end()));
}

int
WindowedMinMax::getMaxMax() const
{
    size_t size = windowCount_->getValue();
    return std::max(*std::min_element(windowSums_.begin(),
                                      windowSums_.begin() + size),
                    *std::min_element(windowSums_.begin() + size,
                                      windowSums_.end())) +
	*std::max_element(windowSums_.begin(), windowSums_.end());
}

void
WindowedMinMax::valueGeneratorChanged(const ValueGeneratorParameter& param)
{
    Logger::ProcLog log("valueGeneratorChanged", getLog());
    LOGINFO << "new: " << param.getValue() << std::endl;
    switch (param.getValue()) {
    case kAverage: valueGeneratorProc_ = &WindowedMinMax::getAverage; break;
    case kMin: valueGeneratorProc_ = &WindowedMinMax::getMin; break;
    case kMax: valueGeneratorProc_ = &WindowedMinMax::getMax; break;
    case kMaxMin: valueGeneratorProc_ = &WindowedMinMax::getMaxMin; break;
    case kMaxMax: valueGeneratorProc_ = &WindowedMinMax::getMaxMax; break;
    default:
	LOGERROR << "invalid selection - " << param.getValue() << std::endl;
	break;
    }
}

void
WindowedMinMax::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kProcessor, valueGenerator_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (! status[WindowedMinMax::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return Algorithm::FormatInfoValue(kValueGeneratorNames[int(status[WindowedMinMax::kProcessor])]);
}

// Factory function for the DLL that will create a new instance of the WindowedMinMax class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
WindowedMinMaxMake(Controller& controller, Logger::Log& log)
{
    return new WindowedMinMax(controller, log);
}
