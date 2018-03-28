#include "StreamStatus.h"

using namespace SideCar::IO;

StreamStatus::StreamStatus(const std::string& name) : StatusBase(kNumSlots, GetClassName(), name)
{
    ;
}
