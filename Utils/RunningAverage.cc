#include "RunningAverage.h"

using namespace Utils;

RunningAverage::RunningAverage(size_t windowSize, double initialValue) :
    values_(windowSize, initialValue), oldest_(0), sum_(0.0)
{
    ;
}

void
RunningAverage::addValue(double value)
{
    sum_ -= values_[oldest_];
    sum_ += value;
    values_[oldest_++] = value;
    if (oldest_ == values_.size()) { oldest_ = 0; }
}

void
RunningAverage::clear(double initialValue)
{
    auto size = values_.size();
    values_.clear();
    values_.resize(size, initialValue);
    sum_ = initialValue * size;
}

void
RunningAverage::setWindowSize(size_t windowSize, double initialValue)
{
    oldest_ = 0;
    values_.resize(windowSize);
    clear(initialValue);
}
