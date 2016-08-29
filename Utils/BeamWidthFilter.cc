#include <ostream>

#include "BeamWidthFilter.h"

using namespace Utils;

double
BeamWidthFilter::add(double beamWidth)
{
    if (beamWidth > 0.0 && (runningSum_ == 0.0 || beamWidth < 2.0 * filterValue_)) {
	runningSum_ -= widths_[nextSlot_];
	runningSum_ += beamWidth;
	widths_[nextSlot_++] = beamWidth;
	if (nextSlot_ == widths_.size()) {
	    filled_ = true;
	    nextSlot_ = 0;
	}

	filterValue_ = runningSum_ / (filled_ ? widths_.size() : nextSlot_);
    }

    return filterValue_;
}

std::ostream&
BeamWidthFilter::print(std::ostream& os) const
{
    for (size_t index = 0; index < widths_.size(); ++index) {
	os << widths_[index] << ' ';
    }
    return os;
}
