#include <algorithm> // for std::transform
#include <cmath>     // for floor
#include <deque>
#include <functional> // for std::bind* and std::mem_fun*

#include "Centroid.h"
#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Extraction.h"
#include "Messages/RadarConfig.h"
#include "ScanLine.h"
#include "Utils/Utils.h"

#include "ExtractWithCentroiding.h"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

ExtractWithCentroiding::ExtractWithCentroiding(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), m_is(log), m_videoHistory(log),
    m_centroidRangeMin(Parameter::DoubleValue::Make("centroidRangeMin", "Centroid Range Min (km)", 7.0)),
    m_centroidAzMin(Parameter::DoubleValue::Make("centroidAzMin", "Centroid Az Min (rad)", 0.30)),
    m_discardRangeMin(Parameter::DoubleValue::Make("discardRangeMin", "Discard Range Min (km)", 120.0)),
    m_discardAzMin(Parameter::DoubleValue::Make("discardAzMin", "Discard Az Min (rad)", M_PI))
{
    ;
}

bool
ExtractWithCentroiding::startup()
{
    registerProcessor<ExtractWithCentroiding, Messages::BinaryVideo>(&ExtractWithCentroiding::process);

    return registerParameter(m_centroidRangeMin) && registerParameter(m_centroidAzMin) &&
           registerParameter(m_discardRangeMin) && registerParameter(m_discardAzMin) && Algorithm::startup();
}

bool
ExtractWithCentroiding::process(const Messages::BinaryVideo::Ref& msg)
{
    static Logger::ProcLog log("process", getLog());

    TargetSize minDiscardTargetSize;
    TargetSize minCentroidSize;

    Messages::Video::Ref video(msg->getVideoBasis());

    // setup centroiding and discard sizes
    minCentroidSize.maxRange =
        (RANGEBIN)((m_centroidRangeMin->getValue() - video->getRangeMin()) / video->getRangeFactor());
    minCentroidSize.maxRangeValid = true;
    minCentroidSize.maxAz = (AZIMUTH)(m_centroidAzMin->getValue());
    minCentroidSize.maxAzValid = true;

    minDiscardTargetSize.maxRange =
        (RANGEBIN)((m_discardRangeMin->getValue() - video->getRangeMin()) / video->getRangeFactor());
    minDiscardTargetSize.maxRangeValid = true;
    minDiscardTargetSize.maxAz = (AZIMUTH)(m_discardAzMin->getValue());
    minDiscardTargetSize.maxAzValid = true;

    // create a vector for extractions
    Messages::Extractions::Ref extractions;

    // process the binary data to update image segmentation (i.e., target
    // detection)
    m_is.AppendScanLine(BinaryScanLineVector(&(msg->getData())), msg->getAzimuthStart(), minDiscardTargetSize);

    // process the video data (i.e., just store it for later use)
    m_videoHistory.Append(video->getData());

    // process every target that is complete (i.e., ready for extraction and
    // further processing)
    while (!m_is.IsClosedTargetsEmpty()) {
        if (!extractions) extractions = Messages::Extractions::Make("Extract", msg);

        SegmentedTargetImagePtr target = m_is.PopClosedTarget();

        TargetSize size = target->GetSize();

        if (!(size > minCentroidSize)) { // notice, this is not the same as size < minCentroidSize

            TargetPosition pos = size.Center();
            float range = msg->getRangeAt(pos.range);
            extractions->push_back(Messages::Extraction(Time::TimeStamp::Now(), range, pos.az, 0.0));
            LOGDEBUG << "target found at az=" << pos.az << " range=" << range << " rangebin=" << pos.range << std::endl;
        } else {
            // this target is too large, send to the centroider for
            //  further resolution

            // extract video and mask. the upper left corner of both the video
            // and mask lies at [minRange, minAz].  the lower right lies at
            // [maxRange, maxAz].  the images are (priCount) x (maxRange-minRange+1)
            // pixels.

            LOGDEBUG << "centroiding a target:" << size << " dRange=" << msg->getRangeAt(size.RangeExtent())
                     << std::endl;

            BinaryTargetImagePtr mask = target->MakeBinaryTargetImage();
            VideoTargetImagePtr video = m_videoHistory.GetWindow(size);
            video->SetAzimuthData(mask->GetAzimuthData());

            // create a centroider
            Centroid centroider(mask, video, getLog());

            centroider.Process();

            while (!centroider.IsClosedTargetsEmpty()) {
                TargetPosition cenPos = centroider.PopClosedTargetPosition();

                float cenRange = msg->getRangeAt(cenPos.range);

                extractions->push_back(Messages::Extraction(Time::TimeStamp::Now(), cenRange, cenPos.az, 0.0));

                LOGDEBUG << "  centroided target found at az=" << cenPos.az << " range=" << cenRange
                         << " rangebin=" << cenPos.range << std::endl;
            }
        }
    }

    // discard any un-need video data
    m_videoHistory.SetDepth(m_is.GetMaxRowDepth());

    // publish the targets
    bool rc = true;
    if (extractions) rc = send(extractions);

    return rc;
}

extern "C" ACE_Svc_Export Algorithm*
ExtractWithCentroidingMake(Controller& controller, Logger::Log& log)
{
    return new ExtractWithCentroiding(controller, log);
}
