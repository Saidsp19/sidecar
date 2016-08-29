#include <algorithm>		// for std::swap

#include "Logger/Log.h"

#include "Despeckle.h"
#include "Despeckle_defaults.h"

using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

Despeckle::Despeckle(Controller& controller, Logger::Log &log)
    : Algorithm(controller, log),
      varianceMultiplier_(Parameter::DoubleValue::Make("varianceMultiplier", "Variance Multiplier",
                                                       kDefaultVarianceMultiplier)),
      past_(3)
{
    ;
}

bool
Despeckle::startup()
{
    registerProcessor<Despeckle,Video>(&Despeckle::process);
    return registerParameter(varianceMultiplier_) && Algorithm::startup();
}

bool
Despeckle::reset()
{
    past_.clear();
    return true;
}

bool
Despeckle::process(Messages::Video::Ref in0)
{
    static Logger::ProcLog log("process", getLog());

    LOGTIN << past_.size() << std::endl;

    past_.add(in0);
    if (! past_.full()) {
        LOGTOUT << std::endl;
	return true;
    }

    size_t gateCount = past_.getMaxMsgSize();
    if (gateCount < 1) {
	LOGERROR << "invalid gate count" << std::endl;
	return true;
    }

    LOGDEBUG << "gate count: " << gateCount << std::endl;

    Messages::Video::Ref in1 = past_[1];
    Messages::Video::Ref in2 = past_[2];
    Messages::Video::Ref out(Messages::Video::Make(getName(), in1));
    out->resize(gateCount, 0);

    VideoT sort[6];
    out[0] = in1[0];
    out[gateCount - 1] = in1[gateCount - 1];

    float varianceMultiplier = varianceMultiplier_->getValue();
    int changedCounter = 0;

    for (size_t index = 1; index < gateCount - 1; ++index) {

	// Find the median of the neighboring PRIs -- use a simple sorting network
	//
        sort[0] = in2[index - 1];
        sort[1] = in2[index];
        sort[2] = in2[index + 1];
        sort[3] = in0[index - 1];
        sort[4] = in0[index];
        sort[5] = in0[index + 1];

        Sort(sort[0], sort[1]);
        Sort(sort[3], sort[4]);
        Sort(sort[0], sort[2]);
        Sort(sort[3], sort[5]);
        Sort(sort[1], sort[2]);
        Sort(sort[4], sort[5]);
        Sort(sort[0], sort[3]);
        Sort(sort[1], sort[4]);
        Sort(sort[2], sort[5]);
        Sort(sort[1], sort[3]);
        Sort(sort[2], sort[4]);

        float median = (sort[2] + sort[3]) * 0.5;

	int value = in1[index];
        if (value > median) {

	    // Calculate the variance about the median
	    //
            float variance = 0;
            for (int i = -1; i < 2; ++i) {
                float a = in2[index + i] - median;
                float b = in0[index + i] - median;
                variance += a * a + b * b;
            }

	    // Final threshold test
	    //
            float dist = value - median;
            if (dist * dist > varianceMultiplier * variance) {
                value = VideoT(::rint(median));
                ++changedCounter;
            }
        }

        out[index] = value;
    }

    LOGINFO << "fixed: " << changedCounter << " of " << gateCount << std::endl;

    auto rc = send(out);
    LOGTOUT << rc << std::endl;

    return rc;
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
DespeckleMake(Controller& controller, Logger::Log& log)
{
    return new Despeckle(controller, log);
}
