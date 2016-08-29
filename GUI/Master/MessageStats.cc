#include "Algorithms/RawPRI/RawPRI.h"
#include "XMLRPC/XmlRpcValue.h"

#include "MessageStats.h"

using namespace SideCar::Algorithms;
using namespace SideCar::GUI::Master;

MessageStats::MessageStats()
    : duplicates_(0), drops_(0), recordingDuplicates_(0),
      recordingDrops_(0)
{
    ;
}

MessageStats::MessageStats(const IO::StatusBase& status)
    : duplicates_(int(status[RawPRI::kDuplicates])),
      drops_(int(status[RawPRI::kDrops])),
      recordingDuplicates_(int(status[RawPRI::kRecordingDuplicates])),
      recordingDrops_(int(status[RawPRI::kRecordingDrops]))
{
    ;
}

MessageStats&
MessageStats::operator+=(const MessageStats& rhs)
{
    duplicates_ += rhs.duplicates_;
    drops_ += rhs.drops_;
    recordingDuplicates_ += rhs.recordingDuplicates_;
    recordingDrops_ += rhs.recordingDrops_;
    return *this;
}
