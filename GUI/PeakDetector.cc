#include <algorithm>

#include "PeakDetector.h"

using namespace SideCar::GUI;

void
PeakDetector::clear(float zero)
{
    ValueDeque zeros(values_.size(), zero);
    values_.swap(zeros);
    peakIndex_ = 0;
}

bool
PeakDetector::add(float value)
{
    // Update the held values, but remember the peak value just in case it is the value we pop (the last in the
    // deque).
    //
    float peakValue(values_[peakIndex_]);
    values_.push_front(value);
    values_.pop_back();
    bool changed = value > peakValue;

    if (changed || value == peakValue) {
        peakIndex_ = 0;
    } else if (peakIndex_ < values_.size() - 1) {
        ++peakIndex_;
    } else {
        peakIndex_ = std::max_element(values_.begin(), values_.end()) - values_.begin();
        value = getValue();
        changed = value < peakValue;
    }

    if (changed) emit newPeak(value);

    return changed;
}
