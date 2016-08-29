#include "boost/bind.hpp"
#include "SegmentSplitter.h"
#include "Messages/RadarConfig.h"
#include "Messages/MetaTypeInfo.h"

using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

static const char* kAlgorithmName = "SegmentSplitter";

SegmentSplitter::SegmentSplitter(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log),
    deltaRange(Parameter::IntValue::Make("range/2", "Range / 2", 3)),
    deltaAz(Parameter::IntValue::Make("azimuth/2", "Azimuth / 2", 6)),
    overlap(Parameter::NormalizedValue::Make("overlap", "Overlap", 0.1)),
    buffer(RadarConfig::GetShaftEncodingMax() + 1,
	   RadarConfig::GetGateCountMax(), 2, 2),
    row(buffer)
{
    overlap->connectChangedSignalTo(
        boost::bind(&SegmentSplitter::handle_overlap_change, this, _1));
    buffer.clearData();
    buffer.clearWindow();
}

bool
SegmentSplitter::startup()
{
    registerProcessor<SegmentSplitter,SegmentMessage>(
	&SegmentSplitter::processSegment);
    registerProcessor<SegmentSplitter,Video>(
	&SegmentSplitter::processVideo);
    return Algorithm::startup();
}

bool
SegmentSplitter::reset()
{
    return true;
}

// private struct for SegmentSplitter::process()
struct Peak
{
    size_t range;
    size_t azimuth;
    Video::DatumType value;
    float sum; // sum of the values for the 3x3 block centered about this peak
};

bool
SegmentSplitter::processSegment(SegmentMessage::Ref pri)
{
    // Read in the message
    const std::list<Segment> &in=pri->data()->data();

    // identify the peaks in this extraction a peak is the highest value in its 9-cell region
    //
    std::list<Segment>::const_iterator seg;
    std::list<Segment>::const_iterator stop=in.end();
    Peak peak;
    std::list<Peak> peaks;
    for(seg=in.begin(); seg!=stop; seg++)
    {
        size_t azimuth=seg->azimuth;

        Video::DatumType oldValue=buffer.get(azimuth, seg->start - 1);
        Video::DatumType value=buffer.get(azimuth, seg->start);
        Video::DatumType newValue=buffer.get(azimuth, seg->start + 1);
        for(size_t i=seg->start; i <= seg->stop; i++)
        {
            if(value>=newValue && value>=oldValue)
            {
                // found a peak on this azimuth, check to either side
                Video::DatumType tmp;
                peak.sum=0;
                if((tmp=buffer.get(azimuth-1, i-1), peak.sum+=tmp, tmp<=value) &&
                   (tmp=buffer.get(azimuth-1,  i), peak.sum+=tmp, tmp<=value) &&
                   (tmp=buffer.get(azimuth-1, i+1), peak.sum+=tmp, tmp<=value) &&
                   (tmp=buffer.get(azimuth+1, i-1), peak.sum+=tmp, tmp<=value) &&
                   (tmp=buffer.get(azimuth+1,  i), peak.sum+=tmp, tmp<=value) &&
                   (tmp=buffer.get(azimuth+1, i+1), peak.sum+=tmp, tmp<=value))
                {
                    // its a local maximum...
                    peak.range=i;
                    peak.azimuth=azimuth;
                    peak.value=value;
                    peak.sum+=oldValue + value + newValue;
                    peaks.push_back(peak);
                }
            }

            // shift
            oldValue=value;
            value=newValue;
            newValue=buffer.get(azimuth, i+1);
        }
    }

    // Determine distance thresholds
    //
    const int wRange=deltaRange->getValue(); // ellipse axes
    const int wAz=deltaAz->getValue();
    const float close=wRange*wAz*minDist; // distance between origins
    const float close2=close*close;
    const int scan = RadarConfig::GetShaftEncodingMax() + 1;
    const int scan_2 = scan / 2;

    // Sort through the peaks, picking the highest peak "per object"
    //
    std::list<Peak> extractions;
    while(peaks.size())
    {
        // Find the highest peak
        std::list<Peak>::iterator peak=peaks.begin();
        std::list<Peak>::iterator max=peak; // start assuming the first peak is highest
        std::list<Peak>::const_iterator stop=peaks.end();
        for(peak++ ; peak!=stop; peak++) // then check the other peaks
        {
            if((peak->value > max->value) ||
               ((peak->value == max->value) && (peak->sum > max->sum)))
            {
                max=peak;
            }
        }

        // declare the extraction
        Peak summit=*max;
        extractions.push_back(summit);

        // remove any peaks shadowed by max
        peaks.erase(max);
        for(peak=peaks.begin(); peak!=stop; /* increment in the loop */)
        {
            int dRange=wAz*(summit.range - peak->range); // cross-multiply instead of dividing
            int dAz=summit.azimuth - peak->azimuth;
            if(dAz < -scan_2)
                dAz+=scan;
            if(dAz > scan_2)
                dAz-=scan;
            dAz*=wRange;

            if(dRange*dRange + dAz*dAz < close2)
            {
                peak=peaks.erase(peak);
            }
            else
            {
                peak++;
            }
        }
    }

    // Create a new SegmentMessage for each extraction
    //
    std::list<Peak>::const_iterator ext;
    std::list<Peak>::const_iterator stopExt=extractions.end();
    Segment s;
    for(ext=extractions.begin(); ext!=stopExt; ext++)
    {
        // create a SegmentMessage
        SegmentMessage::Ref output(
	    new SegmentMessage(kAlgorithmName, pri, pri->getRangeMin(),
                               pri->getRangeFactor()));
        SegmentList &out=*output->data();

        // Set the peak
        out.peakPower=ext->value;
        out.peakRange=ext->range;
        out.peakAzimuth=ext->azimuth;

        // Specify the bounds for this extraction
        size_t az0=ext->azimuth;
        size_t rangeMin=ext->range - wRange;
        size_t rangeMax=ext->range + wRange;

        // scan over the original segments, and select segments which are close to this peak
        // this time, use the 1-norm
        for(seg=in.begin(); seg!=stop; seg++)
        {
            s.azimuth=seg->azimuth;

            int dAz=az0-s.azimuth;
            if(dAz < -scan_2)
                dAz+=scan;
            if(dAz > scan_2)
                dAz-=scan;

            if(abs(dAz) <= wAz)
            {
                s.start=seg->start;
                if(s.start < rangeMin)
                    s.start=rangeMin;
                s.stop=seg->stop;
                if(s.stop > rangeMax)
                    s.stop=rangeMax;

                out.merge(s);
            }
        }

        // send out the SegmentMessage
        if(!send(output))
            return false;
    }

    return true;
}

void SegmentSplitter::handle_overlap_change(
    const Parameter::NormalizedValue& r)
{
    // quadratic fit to the function f : normalized distance -> percent overlap
    // of equal circles
    float a= 0.10672962003465;
    float b=-0.73323282917453;
    float c= 1.01561787789045;
    minDist=(-b - sqrt(b*b - 4*a*(c - r.getValue())))/(2*a);
}

bool
SegmentSplitter::processVideo(Messages::Video::Ref msg)
{
    /// Buffer the new video information
    msg->resize(buffer.getDataCols(), 0);

    vsip::Dense<1, VideoT> mMsg(vsip::Domain<1>(buffer.getDataCols()),
				&msg[0]);
    vsip::Vector<VideoT> vMsg(mMsg);
    mMsg.admit(true);

    currentRow=msg->getShaftEncoding();
    row.setRow(currentRow);
    row.v=vMsg;

    mMsg.release(false);

    return send(msg);
}


// DLL initialization hook
extern "C" ACE_Svc_Export Algorithm*
SegmentSplitterMake(Controller& controller, Logger::Log& log)
{
    return new SegmentSplitter(controller, log);
}
