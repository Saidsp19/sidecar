#include <cmath>

#include "Messages/RadarConfig.h"
#include "Segment2Extraction.h"

using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

static const char* kAlgorithmName = "Segment2Extraction";

Segment2Extraction::Segment2Extraction(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), powerF(Parameter::BoolValue::Make("powerFlag", "Power Flag", true)),
    toBuffer(Parameter::PositiveIntValue::Make("bufferLength", "Buffer Length", 1)),
    extractions(Messages::Extractions::Make(kAlgorithmName, Header::Ref())),
    azFactor(2 * M_PI / (RadarConfig::GetShaftEncodingMax() + 1))
{
    ;
}

bool
Segment2Extraction::startup()
{
    registerProcessor<Segment2Extraction, SegmentMessage>(&Segment2Extraction::process);
    return registerParameter(powerF) && registerParameter(toBuffer) && Algorithm::startup();
}

bool
Segment2Extraction::process(const Messages::SegmentMessage::Ref& pri)
{
    // Read in the message
    const SegmentList& ext = *pri->data();

    if (powerF->getValue()) {
        // use the power centroid
        extractions->push_back(Extraction(Time::TimeStamp::Now(), pri->getRangeAt(ext.centroidRange),
                                          RadarConfig::GetAzimuth(static_cast<uint32_t>(ext.centroidAzimuth)), 0.0));
    } else {
        // use the peak
        extractions->push_back(Extraction(Time::TimeStamp::Now(), pri->getRangeAt(ext.peakRange),
                                          RadarConfig::GetAzimuth(static_cast<uint32_t>(ext.peakAzimuth)), 0.0));
    }

    bool ret = true;
    if (extractions->size() >= size_t(toBuffer->getValue())) {
        ret = send(extractions);
        extractions = Extractions::Make(kAlgorithmName, Header::Ref());
    }
    return ret;
}

// DLL initialization hook
extern "C" ACE_Svc_Export Algorithm*
Segment2ExtractionMake(Controller& controller, Logger::Log& log)
{
    return new Segment2Extraction(controller, log);
}
