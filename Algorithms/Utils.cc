#include "QtCore/QString"

#include "Utils.h"

bool
SideCar::Algorithms::Utils::normalizeSampleRanges(int& start, int& span,
                                                  size_t messageSize)
{
    // If span is <= 0, make it maxSize - span, so that the span size can remain independent of the message
    // size.
    //
    if (span < 1) {
	span += messageSize;
    }

    // If start < 0, then make it messageSize - start so that one can safely specify a starting position from
    // the end of the message, regardless of the message size.
    //
    if (start < 0) {
	start += messageSize;
	if (start < 0) start = 0;
    }

    // Revise the span value so that it only covers valid message elements.
    //
    if (size_t(start + span) > messageSize) {
	span = messageSize - start;
	if (span <= 0)
	    return false;
    }

    return true;
}
