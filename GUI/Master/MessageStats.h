#ifndef SIDECAR_GUI_MASTER_MESSAGESTATS_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_MESSAGESTATS_H

namespace SideCar {
namespace IO {
class StatusBase;
}
namespace GUI {
namespace Master {

class MessageStats {
public:
    MessageStats();

    MessageStats(const IO::StatusBase& status);

    MessageStats& operator+=(const MessageStats& rhs);

    size_t getDuplicates() const { return duplicates_; }

    size_t getDrops() const { return drops_; }

    size_t getRecordingDuplicates() const { return recordingDuplicates_; }

    size_t getRecordingDrops() const { return recordingDrops_; }

private:
    size_t duplicates_;
    size_t drops_;
    size_t recordingDuplicates_;
    size_t recordingDrops_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
