#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "CPIAlgorithm.h"

#include "QtCore/QString"
#include "QtCore/QVariant"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
CPIAlgorithm::CPIAlgorithm(Controller& controller, Logger::Log& log, bool enabled, int cpiSpan) :
    Super(controller, log),
    cpiSpan_(Parameter::PositiveIntValue::Make("cpiSpan", "# of PRIs composing CPI", cpiSpan)),
    dropIncompleteCPI_(Parameter::BoolValue::Make("dropIncompleteCPI",
                                                  "Drop a CPI if any of its PRIs are missing", 0)),
    buffer_(), current_prf_code_(0), beginProcessingCPI_(Time::TimeStamp::Max()),
    enabled_(Parameter::BoolValue::Make("enabled", "Enabled", enabled)), maxMsgSize_(0)
{
    ;
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// CPIAlgorithm class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
CPIAlgorithm::startup()
{
    const IO::Channel& channel(getController().getInputChannel(0));

    if (channel.getTypeKey() == Messages::Video::GetMetaTypeInfo().getKey()) {
        registerProcessor<CPIAlgorithm, Messages::Video>(&CPIAlgorithm::processInputVideo);
    } else if (channel.getTypeKey() == Messages::BinaryVideo::GetMetaTypeInfo().getKey()) {
        registerProcessor<CPIAlgorithm, Messages::BinaryVideo>(&CPIAlgorithm::processInputBinary);
    } else {
        return false;
    }

    // CPIAlgorithms need to manage their own stats because they do not process messages in the "standard"
    // fashion.
    //
    getController().setStatsManaged(false);

    return registerParameter(enabled_) && registerParameter(dropIncompleteCPI_) && registerParameter(cpiSpan_) &&
           Super::startup();
}

bool
CPIAlgorithm::shutdown()
{
    buffer_.clear();
    return Super::shutdown();
}

bool
CPIAlgorithm::processInputVideo(const Messages::Video::Ref& msg)
{
    return processInput(msg);
}

bool
CPIAlgorithm::processInputBinary(const Messages::BinaryVideo::Ref& msg)
{
    return processInput(msg);
}

bool
CPIAlgorithm::processInput(const Messages::PRIMessage::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());
    LOGINFO << std::endl;

    bool rc = true;
    LOGDEBUG << "process msg: " << msg->getRIUInfo().sequenceCounter << std::endl;

    uint32_t prf_code = msg->getRIUInfo().prfEncoding;
    size_t cpiSpan = cpiSpan_->getValue();

    // Handle special case: the arrival of the very first message
    //
    if (!current_prf_code_) current_prf_code_ = prf_code;
    size_t num_msgs = buffer_.size();

    // Look for a change in the prf encoding. This indicates the arrival of the first PRI message of a new CPI.
    //
    if (prf_code != current_prf_code_) {
        beginProcessingCPI_ = Time::TimeStamp::Now();
        bool missing = false;
        if (dropIncompleteCPI_->getValue()) {
            uint32_t lastSeq = (*buffer_.begin())->getSequenceCounter();
            for (MessageQueue::iterator itr = buffer_.begin() + 1; itr != buffer_.end() && !missing; ++itr) {
                missing = missing || ((*itr)->getSequenceCounter() - lastSeq != 1);
                lastSeq = (*itr)->getSequenceCounter();
            }
        }

        if (!missing) rc = processCPI();

        maxMsgSize_ = 0;
        buffer_.clear();
        current_prf_code_ = prf_code;

        // Delta represents the amount of time it took to process the CPI, since our stats are reported in PRI
        // messages / unit time, need to distribute time costs across number of PRIs forming the CPI.
        //
        Time::TimeStamp delta = Time::TimeStamp::Now() - beginProcessingCPI_;
        delta *= double(1.0 / cpiSpan);

        // Add a stat sample for each PRI message in the CPI because data rates are recorded at the PRI message
        // rate.
        //
        for (size_t i = 0; i < cpiSpan; i++) getController().addProcessingStatSample(delta);
    }

    // Buffer PRI messages belonging to the current CPI until the first message of the next CPI arrives.
    //
    buffer_.push_back(msg);
    if (msg->size() > maxMsgSize_) maxMsgSize_ = msg->size();

    return rc;
}

void
CPIAlgorithm::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

QString
CPIAlgorithm::GetFormattedStats(const IO::StatusBase& status)
{
    if (!status[kEnabled]) return "Disabled  ";
    return " ";
}
