#include "boost/bind.hpp"

#include "Logger/Log.h"
#include "Messages/RadarConfig.h"
#include "Messages/Video.h"
#include "Utils/VsipVector.h"

#include "VideoInterpolation.h"
#include "VideoInterpolation_defaults.h"

using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

VideoInterpolation::VideoInterpolation(Controller& controller,
                                       Logger::Log& log)
    : Algorithm(controller, log),
      interpolationCount_(Parameter::PositiveIntValue::Make(
                              "interpolationCount",
                              "Interpolation Count",
                              kDefaultInterpolationCount)),
      past_(2)
{
    ;
}

bool
VideoInterpolation::startup()
{
    registerProcessor<VideoInterpolation,Video>(&VideoInterpolation::process);
    return registerParameter(interpolationCount_) && Algorithm::startup();
}

bool
VideoInterpolation::reset()
{
    past_.clear();
    return true;
}

bool
VideoInterpolation::process(const Messages::Video::Ref& in)
{
    Logger::ProcLog log("process", getLog());

    const float kEncoded2PI = RadarConfig::GetShaftEncodingMax() + 1.0;
    const float kEncodedPI = kEncoded2PI / 2.0;
    
    past_.add(in);
    if (! past_.full()) {
	return send(in);
    }

    // Convention: i indexes the input shaft encoding, j indexes the output shaft encoding
    //
    float iNew = past_[0]->getShaftEncoding();
    float iOld = past_[1]->getShaftEncoding();
    LOGDEBUG << "iNew: " << iNew << " iOld: " << iOld << std::endl;

    if (iNew < iOld) {
	iOld -= kEncoded2PI;
	LOGDEBUG << "unwrapping iOld: " << iOld << std::endl;
    }

    float delta = iNew - iOld;
    LOGDEBUG << "delta: " << delta << std::endl;

    // Block time reversals (i.e. out-of-order packets)
    //
    if (delta > kEncodedPI) {
	LOGERROR << "detected backward movement" << std::endl;
	past_.add(past_[1]);
        return true;
    }

    int emitCount = interpolationCount_->getValue() + 1;
    delta /= emitCount;

    LOGDEBUG << "interpolation delta: " << delta << std::endl;

    // VSIP bindings for the inputs
    VsipVector<Video> vNew(*past_[0]);
    VsipVector<Video> vOld(*past_[1]);
    vNew.admit(true);
    vOld.admit(true);

    vsip::Vector<float> tmp(in->size(), 0.0);

    for (int index = 1; index <= emitCount; ++index) {

	float shaftEncoding = iOld + delta * index;
	if (shaftEncoding < 0.0)
	    shaftEncoding += kEncoded2PI;
	else if (shaftEncoding >= kEncoded2PI)
	    shaftEncoding -= kEncoded2PI;

	LOGDEBUG << "shaftEncoding: " << shaftEncoding << std::endl;

        Video::Ref out(Video::Make(getName(), in));
        out->resize(in->size(), 0);
        out->getRIUInfo().shaftEncoding = size_t(::rint(shaftEncoding));

        VsipVector<Video> vOut(*out);
        vOut.admit(false);

        float weight = float(index) / float(emitCount);
	LOGDEBUG << "index: " << index << " weight: " << weight << std::endl;

	tmp = mul(vOld.v, 1.0 - weight);
	tmp = ma(vNew.v, weight, tmp);
        vOut.v = vsip::impl::view_cast<Video::DatumType>(tmp);
        vOut.release(true);

        if (! send(out))
	    return false;
    }

    vNew.release(false);
    vOld.release(false);

    LOGDEBUG << "done" << std::endl;

    return true;
}


// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
VideoInterpolationMake(Controller& controller, Logger::Log& log)
{
    return new VideoInterpolation(controller, log);
}
