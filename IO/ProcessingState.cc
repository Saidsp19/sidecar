#include <algorithm>
#include <string>

#include "ProcessingState.h"

namespace SideCar {
namespace IO {

struct ProcessingState::Initializer
{
    Initializer();
};

}} 				// end namespace SideCar::IO

using namespace SideCar::IO;

const char* const ProcessingState::displayNames_[kNumStates] =
{
    "---",
    "Init",
    "AutoDiag",
    "Calibrate",
    "Run",
    "Stop",
    "FAILURE"
};

std::vector<std::string> ProcessingState::searchNames_;

ProcessingState::Initializer::Initializer()
{
    if (searchNames_.empty()) {
	searchNames_.resize(kNumStates);
	for (int index = kInvalid; index < kNumStates; ++index) {
	    std::string name(displayNames_[index]);
	    std::transform(name.begin(), name.end(), name.begin(), &tolower);
	    searchNames_[index] = name;
	}
    }
}

ProcessingState::Initializer ProcessingState::initializer_;

const char* const*
ProcessingState::GetNames()
{
    return displayNames_;
}

const char*
ProcessingState::GetName(Value state)
{
    return displayNames_[state];
}

ProcessingState::Value
ProcessingState::GetValue(std::string name)
{
    std::transform(name.begin(), name.end(), name.begin(), &tolower);
    for (int index = kInvalid; index < kNumStates; ++index) {
	if (name == searchNames_[index]) return Value(index);
    }

    return kInvalid;
}
