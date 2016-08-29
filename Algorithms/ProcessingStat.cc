#include "ProcessingStat.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

void
ProcessingStat::reset()
{
    orderedStats_.clear();
    beginProcessing_ = Time::TimeStamp::Max();
}

void
ProcessingStat::endProcessing()
{
    if (beginProcessing_ != Time::TimeStamp::Max()) {
	Time::TimeStamp delta = Time::TimeStamp::Now();
	delta -= beginProcessing_;
	addSample(delta);
    }
}
