#include "AzimuthSweep.h"
#include "Utils.h"

using namespace Utils;

AzimuthSweep::AzimuthSweep(double start, double width)
    : start_(Utils::normalizeRadians(start)), end_(-1.0), width_(width)
{
    ;
}

double
AzimuthSweep::getEnd() const
{
    if (end_ == -1.0) end_ = Utils::normalizeRadians(start_ + width_);
    return end_;
}

bool
AzimuthSweep::overlaps(const AzimuthSweep& other) const
{
    return containsStart(other.start_) || (other.width_ && containsEnd(other.getEnd())) || other.contains(*this);
}

bool
AzimuthSweep::contains(const AzimuthSweep& other) const
{
    return containsStart(other.start_) && containsEnd(other.getEnd());
}

bool
AzimuthSweep::containsStart(double angle) const
{
    return (angle < start_) ? containsStart(angle + kCircleRadians) :
        (angle >= start_ && angle < start_ + width_);
}

bool
AzimuthSweep::containsEnd(double angle) const
{
    return (angle <= start_) ? containsEnd(angle + kCircleRadians) :
	(angle > start_ && angle <= start_ + width_);
}
