#include "boost/bind.hpp"

#include "Utils/VsipVector.h"

#include "Difference.h"
#include "Difference_defaults.h"

using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

Difference::Difference(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log),
    bufferSize_(Parameter::PositiveIntValue::Make("bufferSize", "Buffer Size", kDefaultBufferSize)),
    in0_(bufferSize_->getValue()), in1_(bufferSize_->getValue())
{
    bufferSize_->connectChangedSignalTo(boost::bind(&Difference::bufferSizeChanged, this, _1));
}

bool
Difference::startup()
{
    registerProcessor<Difference, Video>(0, &Difference::processIn0);
    registerProcessor<Difference, Video>(1, &Difference::processIn1);
    return registerParameter(bufferSize_);
}

bool
Difference::reset()
{
    in0_.clear();
    in1_.clear();
    return true;
}

bool
Difference::processIn0(Video::Ref in)
{
    in0_.add(in);
    Video::Ref in1 = in1_.find(in->getSequenceCounter());
    if (!in1) return true;
    return process(in, in1);
}

bool
Difference::processIn1(Video::Ref in)
{
    in1_.add(in);
    Video::Ref in0 = in0_.find(in->getSequenceCounter());
    if (!in0) return true;
    return process(in0, in);
}

bool
Difference::process(Video::Ref in0, Video::Ref in1)
{
    Video::Ref out(Video::Make(getName(), in0));
    out->resize(in0->size(), 0);

    // Calculate in0 - in1
    //
    VsipVector<Video> vIn0(*in0);
    vIn0.admit(true);
    VsipVector<Video> vIn1(*in1);
    vIn1.admit(true);
    VsipVector<Video> vOut(*out);
    vOut.admit(false);
    vOut.v = vIn0.v;
    vIn0.release(false);
    vOut.v -= vIn1.v;
    vIn1.release(false);
    vOut.release(true);

    return send(out);
}

void
Difference::bufferSizeChanged(const Parameter::PositiveIntValue& value)
{
    in0_.setCapacity(value.getValue());
    in0_.clear();
    in1_.setCapacity(value.getValue());
    in1_.clear();
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
DifferenceMake(Controller& controller, Logger::Log& log)
{
    return new Difference(controller, log);
}
