#ifndef SIDECAR_GUI_PPIDISPLAY_BINARYDECIMATOR_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_BINARYDECIMATOR_H

#include "Messages/BinaryVideo.h"

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

/** Simple data decimator for SideCar BinaryVideo messages. Decimates by returning a true value if any of the
    samples within the span defined by the decimation factor is true. A decimation factor of 1 results in no
    decimation. A decimation factor of 2 will return the maximum of every two samples.
*/
class BinaryDecimator {
public:
    BinaryDecimator(int factor, const Messages::BinaryVideo::Ref& msg) :
        factor_(factor), rangeOffset_(factor * msg->getRangeFactor() / 2), msg_(msg), pos_(msg->begin()),
        end_(msg->end())
    {
    }

    operator bool()
    {
        if (pos_ == end_) return false;
        range_ = msg_->getRangeAt(pos_) + rangeOffset_;
        if (factor_ == 1) {
            value_ = *pos_++;
        } else {
            value_ = false;
            for (int count = 1; pos_ < end_ && count < factor_; ++count, ++pos_) {
                if (*pos_) {
                    value_ = true;
                    break;
                }
            }
        }
        return true;
    }

    double getRange() const { return range_; }

    bool getValue() const { return value_; }

private:
    int factor_;
    double rangeOffset_;
    Messages::BinaryVideo::Ref msg_;
    Messages::BinaryVideo::const_iterator pos_;
    Messages::BinaryVideo::const_iterator end_;
    double range_;
    bool value_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
