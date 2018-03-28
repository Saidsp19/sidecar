#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"

#include "CFAR.h"
#include "CFAR_defaults.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

CFAR::CFAR(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), alpha_(Parameter::DoubleValue::Make("alpha", "Alpha", kDefaultAlpha)), videoBuffer_(100)
{
    reset();
}

bool
CFAR::startup()
{
    registerProcessor<CFAR, Video>("estimate", &CFAR::processEstimate);
    registerProcessor<CFAR, Video>("video", &CFAR::processVideo);
    return registerParameter(alpha_) && Algorithm::startup();
}

bool
CFAR::reset()
{
    static Logger::ProcLog log("reset", getLog());
    videoBuffer_.clear();
    return true;
}

bool
CFAR::processEstimate(const Video::Ref& estMsg)
{
    static Logger::ProcLog log("processEstimate", getLog());
    uint32_t sequenceCounter = estMsg->getSequenceCounter();
    LOGINFO << sequenceCounter << std::endl;

    Video::Ref vidMsg;
    while (!videoBuffer_.empty()) {
        Video::Ref tmp(videoBuffer_.back());
        videoBuffer_.pop_back();
        if (tmp->getSequenceCounter() == sequenceCounter) {
            vidMsg = tmp;
            break;
        }
    }

    if (!vidMsg) return true;

    // Normalize the size of the messages.
    //
    size_t vidSize = vidMsg->size();
    size_t estSize = estMsg->size();
    if (vidSize < estSize) {
        vidMsg->resize(estSize);
        vidSize = estSize;
    } else if (estSize < vidSize) {
        estMsg->resize(vidSize);
        estSize = vidSize;
    }

    // Create the output message.
    //
    BinaryVideo::Ref outMsg(BinaryVideo::Make(getName(), vidMsg));
    outMsg->resize(vidSize);

    // Calculate binary samples by comparing video samples against calculated threshold.
    //
    double alpha = alpha_->getValue();
    for (size_t index = 0; index < vidSize; ++index) {
        double threshold = alpha * estMsg[index];
        outMsg[index] = (vidMsg[index] > threshold) ? true : false;
    }

    bool rc = send(outMsg);
    LOGDEBUG << "send: " << rc << std::endl;

    return rc;
}

bool
CFAR::processVideo(const Video::Ref& vidMsg)
{
    static Logger::ProcLog log("processVideo", getLog());
    LOGINFO << vidMsg->getSequenceCounter() << std::endl;
    videoBuffer_.add(vidMsg);
    return true;
}

extern "C" ACE_Svc_Export Algorithm*
CFARMake(Controller& controller, Logger::Log& log)
{
    return new CFAR(controller, log);
}
