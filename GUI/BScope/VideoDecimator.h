#ifndef SIDECAR_GUI_BSCOPE_VIDEODECIMATOR_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_VIDEODECIMATOR_H

#include "Messages/Video.h"

namespace SideCar {
namespace GUI {
namespace BScope {

/** Simple data decimator for SideCar Video messages. Decimates by returning the maximum value found within the
    span of samples defined by the decimation factor. A decimation factor of 1 results in no decimation. A
    decimation factor of 2 will return the maximum of every two samples.
*/
class VideoDecimator
{
public:

    VideoDecimator(int factor, const Messages::Video::Ref& msg)
	: factor_(factor), rangeOffset_(factor * msg->getRangeFactor() / 2),
	  msg_(msg), pos_(msg->begin()), end_(msg->end()) {}

    operator bool()
	{
	    if (pos_ == end_) return false;
	    range_ = msg_->getRangeAt(pos_) + rangeOffset_;
	    value_ = *pos_++;
	    for (int count = 1; pos_ < end_ && count < factor_;
                 ++count, ++pos_)
		if (value_ < *pos_)
		    value_ = *pos_;
	    return true;
	}

    double getRange() const { return range_; }

    int getValue() const { return value_; }

private:
    int factor_;
    double rangeOffset_;
    Messages::Video::Ref msg_;
    Messages::Video::const_iterator pos_;
    Messages::Video::const_iterator end_;
    double range_;
    int value_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
