#include <algorithm> // for std::fill

#include "CollectionStats.h"

using namespace SideCar::GUI::Master;

CollectionStats::CollectionStats() :
    counters_(kNumStats, 0), processingState_(IO::ProcessingState::kInvalid), error_("")
{
    ;
}

void
CollectionStats::clear()
{
    std::fill(counters_.begin(), counters_.end(), 0);
    processingState_ = IO::ProcessingState::kInvalid;
    error_.clear();
}

CollectionStats&
CollectionStats::operator+=(const CollectionStats& rhs)
{
    for (size_t index = 0; index < kNumStats; ++index) counters_[index] += rhs.counters_[index];

    if (processingState_ == IO::ProcessingState::kInvalid)
        processingState_ = rhs.processingState_;
    else if (processingState_ != rhs.processingState_)
        processingState_ = IO::ProcessingState::kFailure;

    if (error_.isEmpty() && !rhs.error_.isEmpty()) error_ = rhs.error_;

    return *this;
}
