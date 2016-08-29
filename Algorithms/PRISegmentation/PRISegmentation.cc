#include "Messages/Segments.h"
#include "PRISegmentation.h"

using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

PRISegmentation::PRISegmentation(Controller& controller, Logger::Log &log)
    : Algorithm(controller, log),
      threshold_(Parameter::IntValue::Make("threshold", "Threshold", 5000))
{
    ;
}

bool
PRISegmentation::startup()
{
    registerProcessor<PRISegmentation,Messages::Video>(
	&PRISegmentation::process);
    return registerParameter(threshold_) && Algorithm::startup();
}

bool
PRISegmentation::reset()
{
    busy_ = false;
    return true;
}

bool
PRISegmentation::process(const Video::Ref& pri)
{
    SegmentMessage::Ref output(new SegmentMessage("PRISegmentation",
                                                  pri,
                                                  pri->getRangeMin(),
                                                  pri->getRangeFactor()));
    SegmentList& out = *output->data();

    bool pending = false;
    Segment s;
    s.azimuth = pri->getShaftEncoding();
    size_t length = pri->size();
    Video::DatumType threshold = threshold_->getValue();
    for (size_t i = 0; i < length; ++i) {
        if (pri[i] >= threshold) {
            if(! pending) {
                pending = true;
                s.start = i;
            }
        }
        else {
            if (pending) {
                pending = false;
                s.stop = i - 1;
                out.merge(s);
            }
        }
    }

    if (pending) {
        s.stop = length - 1;
        out.merge(s);
    }

    if (out.data().size()) {
        busy_ = true;
        return send(output);
    }
    else if (busy_) {
        busy_ = false;
        return send(output);
    }

    return true;
}

extern "C" ACE_Svc_Export Algorithm*
PRISegmentationMake(Controller& controller, Logger::Log& log)
{
    return new PRISegmentation(controller, log);
}
