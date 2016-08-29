#include "boost/bind.hpp"

#include <algorithm>		// for std::transform
#include <functional>		// for std::bind* and std::mem_fun*

#include "Logger/Log.h"
#include <vsip/math.hpp>

#include "Channel.h"
#include "DownConverter.h"
#include "DownConverter_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::DownConverterUtils;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
DownConverter::DownConverter(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log),
      maxBufferSize_(Parameter::PositiveIntValue::Make(
                         "maxBufferSize", "Max channel buffer size",
                         kDefaultMaxBufferSize)),  
      alpha_(Parameter::DoubleValue::Make(
                 "alpha", "Alpha value for low-pass filter",
                 kDefaultAlpha)),  
      enabled_(Parameter::BoolValue::Make("enabled", "Enabled",
                                          kDefaultEnabled))
{
    maxBufferSize_->connectChangedSignalTo(
	boost::bind(&DownConverter::maxBufferSizeChanged, this, _1));
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// DownConverter class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
DownConverter::startup()
{
    bool ok = true;
    
    Logger::ProcLog log("startup", getLog());
    
    cohoChannel_ = new Channel(*this, maxBufferSize_->getValue());
    addProcessor("coho", Messages::Video::GetMetaTypeInfo(), cohoChannel_);
    
    rxChannel_ = new Channel(*this, maxBufferSize_->getValue());
    addProcessor("receive", Messages::Video::GetMetaTypeInfo(), rxChannel_);
    
    return ok &&
	registerParameter(enabled_) &&
	registerParameter(maxBufferSize_) &&
	registerParameter(alpha_) &&
	Algorithm::startup();
}

bool
DownConverter::process()
{
    
    static Logger::ProcLog log("process", getLog());

    LOGINFO << std::endl;

    if (! enabled_->getValue()) {
	if (rxChannel_->isEmpty())
            return true;
	uint32_t rxSeq = rxChannel_->getNextSequence();
	cohoChannel_->prune(rxSeq);
	return send(rxChannel_->getData());
    }
  
    if (rxChannel_->isEmpty() || cohoChannel_->isEmpty()) {
	LOGDEBUG << "empty stream" << std::endl;
	return true;
    }

    // Locate the next common message sequence in the two input queues
    //
    uint32_t rxSeq   = rxChannel_->getNextSequence();
    uint32_t cohoSeq = cohoChannel_->getNextSequence();
 
    if (cohoChannel_->prune(rxSeq) || rxChannel_->prune(cohoSeq) ||
        rxChannel_->getNextSequence() != cohoChannel_->getNextSequence()) {
	LOGDEBUG << "empty stream" << std::endl;
	return true;
    }

    // We have a common message in the two channels so we can generate output
    //
    Messages::Video::Ref cohoMsg(cohoChannel_->getData());
    Messages::Video::Ref rxMsg(rxChannel_->getData());

    cohoChannel_->popData();
    rxChannel_->popData();

    Messages::Video::Ref outMsg(Messages::Video::Make(getName(), rxMsg));
    Messages::Video::Container& out(outMsg->getData());

    int num_samples = cohoMsg->size() / 2;
  
    VsipComplexVector cohoVec(num_samples);
    VsipComplexVector rxVec(num_samples);


    // populate the vectors
    for(int i = 0; i < num_samples; i++) {
        rxVec.put(i, ComplexType(rxMsg[2*i], rxMsg[2*i+1]));
        cohoVec.put(i, ComplexType(cohoMsg[2*i], cohoMsg[2*i+1]));
    }

    // apply a low pass filter
    float alpha = alpha_->getValue();
    for(int i = 1; i < num_samples; i++) {
	rxVec(i) = rxVec(i - 1) + alpha*(rxVec(i) - rxVec(i - 1));
    }

    // compute the average magnitude in the coho vector
    float avg_mag = vsip::sqrt(vsip::meansqval(cohoVec));


    // perform down conversion
    rxVec   = rxVec*(conj(cohoVec) / avg_mag);

    // push the data back out
    for(int i = 0; i < num_samples; i++) {
        out.push_back(Messages::Video::DatumType(::rint(rxVec(i).real())));
        out.push_back(Messages::Video::DatumType(::rint(rxVec(i).imag())));
    }

    return send(outMsg);
}

void
DownConverter::maxBufferSizeChanged(
    const Parameter::PositiveIntValue& parameter)
{
    Logger::ProcLog log("maxBufferSizeChanged", getLog());
    LOGINFO << "maxBufferSize: " << parameter.getValue() << std::endl;
    cohoChannel_->setMaxBufferSize(parameter.getValue());
    rxChannel_->setMaxBufferSize(parameter.getValue());
}

// Factory function for the DLL that will create a new instance of the DownConverter class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
DownConverterMake(Controller& controller, Logger::Log& log)
{
    return new DownConverter(controller, log);
}
