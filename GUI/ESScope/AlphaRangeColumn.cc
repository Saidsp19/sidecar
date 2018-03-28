#include "AlphaRangeColumn.h"
#include "RadarSettings.h"

using namespace SideCar;
using namespace SideCar::GUI::ESScope;

void
AlphaRangeColumn::update(const Messages::Video::Ref& msg, const RadarSettings* radarSettings, int scan)
{
    if (empty()) {
        resize(radarSettings->getRangeScans());
        scan_ = -1;
    }

    const Container& data(msg->getData());

    if (scan_ != scan) {
        scan_ = scan;
        for (size_t index = 0; index < data.size(); ++index) {
            int bin = radarSettings->getSampleRangeIndex(index);
            data_[bin] = data[index];
        }
    } else {
        for (size_t index = 0; index < data.size(); ++index) {
            int bin = radarSettings->getSampleRangeIndex(index);
            if (data[index] > data_[bin]) { data_[bin] = data[index]; }
        }
    }
}
