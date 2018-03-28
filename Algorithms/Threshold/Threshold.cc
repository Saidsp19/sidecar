#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "boost/bind.hpp"

#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"

#include "Threshold.h"
#include "Threshold_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

Threshold::Threshold(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), threshold_(Parameter::IntValue::Make("threshold", "Threshold", kDefaultThreshold))
{
    threshold_->connectChangedSignalTo(boost::bind(&Threshold::thresholdChanged, this, _1));
}

bool
Threshold::startup()
{
    registerProcessor<Threshold, Video>(&Threshold::process);
    thresholdValue_ = threshold_->getValue();
    return registerParameter(threshold_) && Algorithm::startup();
}

/** Functor that performs a threshold check on given values to see if they are >= a threshold value.
 */
struct ThresholdFilter {
    Threshold::DatumType threshold_;
    ThresholdFilter(Threshold::DatumType v) : threshold_(v) {}
    bool operator()(Threshold::DatumType v) const { return v >= threshold_; }
};

bool
Threshold::process(const Video::Ref& in)
{
    static Logger::ProcLog log("process", getLog());
    LOGDEBUG << std::endl;

    BinaryVideo::Ref out(BinaryVideo::Make(getName(), in));

    // Fill output message with boolean values that represent whether or not sample values were >= thresholdValue_.
    //
    std::transform(in->begin(), in->end(), std::back_inserter<>(out->getData()), ThresholdFilter(thresholdValue_));

    LOGDEBUG << *out.get() << std::endl;
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
Threshold::thresholdChanged(const Parameter::IntValue& value)
{
    thresholdValue_ = value.getValue();
}

void
Threshold::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kThreshold, thresholdValue_);
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    int threshold = status[Threshold::kThreshold];
    return Algorithm::FormatInfoValue(QString("Threshold: %1").arg(threshold));
}

extern "C" ACE_Svc_Export Algorithm*
ThresholdMake(Controller& controller, Logger::Log& log)
{
    return new Threshold(controller, log);
}
