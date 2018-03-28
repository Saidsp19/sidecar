#include "Algorithms/Controller.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"

#include "EdgeDetector.h"
#include "EdgeDetector_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kEdgeNames[] = {
    "Leading",
    "Trailing",
};

const char* const*
EdgeDetector::EdgeEnumTraits::GetEnumNames()
{
    return kEdgeNames;
}

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
EdgeDetector::EdgeDetector(Controller& controller, Logger::Log& log) :
    Super(controller, log), edge_(EdgeParameter::Make("edge", "Edge", Edge(kDefaultEdge))), detectedCount_(0),
    examinedCount_(0)
{
    ;
}

bool
EdgeDetector::startup()
{
    registerProcessor<EdgeDetector, Messages::Video>(&EdgeDetector::processInput);
    return registerParameter(edge_) && Super::startup();
}

bool
EdgeDetector::reset()
{
    detectedCount_ = 0;
    examinedCount_ = 0;
    return Super::reset();
}

bool
EdgeDetector::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    Messages::BinaryVideo::Ref out(Messages::BinaryVideo::Make("EdgeDetector", msg));

    // We need three points to work with: prev - previous point curr - current point next - next point Thus, we
    // won't have a calculated value for the first or last sample in the input message; these values will be
    // false in the output.
    //
    Messages::Video::const_iterator next = msg->begin();
    Messages::Video::const_iterator prev = next++;
    Messages::Video::const_iterator curr = next++;
    Messages::Video::const_iterator end = msg->end();

    out->push_back(false);
    if (edge_->getValue() == kLeading) {
        while (next < end) {
            bool v = *prev < *curr && *curr <= *next;
            if (v) ++detectedCount_;
            ++examinedCount_;
            ++prev;
            ++curr;
            ++next;
            out->push_back(v);
        }
    } else {
        while (next < end) {
            bool v = *prev >= *curr && *curr > *next;
            if (v) ++detectedCount_;
            ++examinedCount_;
            ++prev;
            ++curr;
            ++next;
            out->push_back(v);
        }
    }

    out->push_back(false);

    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
EdgeDetector::setDetectionType(Edge type)
{
    edge_->setValue(type);
}

void
EdgeDetector::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEdge, edge_->getValue());
    status.setSlot(kDetectedCount, int(detectedCount_));
    status.setSlot(kExaminedCount, int(examinedCount_));
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;

    // Format status information here.
    //
    int detectedCount = status[EdgeDetector::kDetectedCount];
    int examinedCount = status[EdgeDetector::kExaminedCount];
    double percentage = 0.0;
    if (examinedCount) percentage = (detectedCount * 100.0) / examinedCount;

    return Algorithm::FormatInfoValue(
        QString("%1  %3%").arg(kEdgeNames[int(status[EdgeDetector::kEdge])]).arg(percentage, 0, 'f', 3));
}

// Factory function for the DLL that will create a new instance of the EdgeDetector class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
EdgeDetectorMake(Controller& controller, Logger::Log& log)
{
    return new EdgeDetector(controller, log);
}
