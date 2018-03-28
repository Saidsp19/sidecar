#include "SegmentStats.h"
#include "Messages/MetaTypeInfo.h"
#include "Messages/RadarConfig.h"
#include "boost/bind.hpp"
using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

SegmentStats::SegmentStats(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), deltaRange(Parameter::IntValue::Make("range/2", "Range / 2", 3)),
    deltaAz(Parameter::IntValue::Make("azimuth/2", "Azimuth / 2", 6)),
    maxRangeDrop(Parameter::NormalizedValue::Make("maxRangeDrop", "Max Range Drop", 0.5)),
    maxAzDrop(Parameter::NormalizedValue::Make("maxAzimuthDrop", "Max Azimuth Drop", 0.5)),
    minPower(Parameter::NormalizedValue::Make("minPower", "Min Power", 0.5)),
    azimuthEncodings(RadarConfig::GetShaftEncodingMax() + 1), rangeGates(RadarConfig::GetGateCountMax()),
    buffer(azimuthEncodings, rangeGates, deltaAz->getValue(), deltaRange->getValue()), row(buffer)
{
    // use a circular buffer to source the video data
}

bool
SegmentStats::startup()
{
    registerProcessor<SegmentStats, SegmentMessage>(&SegmentStats::processSegmentMessage);
    registerProcessor<SegmentStats, Video>(&SegmentStats::processVideo);

    return registerParameter(deltaRange) && registerParameter(deltaAz) && registerParameter(maxRangeDrop) &&
           registerParameter(maxAzDrop) && registerParameter(minPower) && Algorithm::startup();
}

bool
SegmentStats::processSegmentMessage(SegmentMessage::Ref pri)
{
    // Read in the message
    SegmentList& in = *pri->data();

    // Create a new message to send
    SegmentMessage::Ref output(new SegmentMessage("SegmentStats", pri, pri->getRangeMin(), pri->getRangeFactor()));
    SegmentList& out = *output->data();

    // calculate the statistics in one pass
    //

    /*
      Note on handling wraparound:
      Normally one calculates the first moment by finding sum(bin*position)/sum(bin).
      This doesn't work when there is a discontinuity (such as at 0==2pi).
      Instead, calculate two independent first moments, one for [0,pi) and another for [pi,2pi).
      Then look at the angle between these two moments to determine how things should resolve.
      Only fails if the angle exceeds pi, which won't happen for a well-formed target.
    */

    // Calculate the total power
    const size_t pi = (RadarConfig::GetShaftEncodingMax() + 1) / 2;
    double powerA = 0; // for [0,pi)
    double powerB = 0; // for [pi,2pi)

    // Calculate the power centroid
    double powerAx = 0;
    double powerBx = 0;
    double powery = 0; // range doesn't wrap

    // Find the peak power
    VideoT peak = std::numeric_limits<VideoT>::min();
    size_t peakRange = 0, peakAz = 0;
    if (in.peakPower > peak) {
        peak = in.peakPower;
        peakRange = in.peakRange;
        peakAz = in.peakAzimuth;
    }

    // The master loop
    std::list<Segment>::const_iterator seg;
    std::list<Segment>::const_iterator stop = in.data().end();
    for (seg = in.data().begin(); seg != stop; seg++) {
        out.merge(*seg); // Add this segment to the output

        size_t azimuth = seg->azimuth;
        for (size_t i = seg->start; i < seg->stop; i++) {
            VideoT p = buffer.get(azimuth, i);
            if (azimuth < pi) {
                powerA += p;
                powerAx += p * azimuth;
            } else {
                powerB += p;
                powerBx += p * azimuth;
            }
            powery += p * i;

            if (p > peak) {
                peak = p;
                peakRange = i;
                peakAz = azimuth;
            }
        }
    }

    // Resolve the power centroid
    const double dpi = (RadarConfig::GetShaftEncodingMax() + 1) / 2;
    double power = powerA + powerB;
    double powerx = (powerAx + powerBx) / power;
    if (powerA && powerB) {
        powerAx /= powerA;
        powerBx /= powerB;

        if (powerBx - powerAx >= dpi) {
            if (powerA >= powerB) {
                powerx -= dpi;
            } else {
                powerx += dpi;
            }
        }
    }
    powery /= power;

    out.totalPower = power;
    out.centroidRange = powery;
    out.centroidAzimuth = powerx;

    out.peakPower = peak;
    out.peakRange = peakRange;
    out.peakAzimuth = peakAz;

    // Check for sharp fall-off at the edges ?? Average over a few PRI's to avoid modulation noise ??
    //

    size_t meanx = size_t(powerx + 0.5);
    size_t meany = size_t(powery + 0.5);
    const VideoT thresh = VideoT(power * minPower->getValue() / out.getCellCount());
    out.distMinRange = findRangeEnd(meanx, meany, -deltaRange->getValue(), -1, thresh);
    out.distMaxRange = findRangeEnd(meanx, meany, deltaRange->getValue(), rangeGates, thresh);
    out.distMinAzimuth = findAzEnd(meany, meanx, -deltaAz->getValue(), -1, azimuthEncodings - 1, thresh);
    out.distMaxAzimuth = findAzEnd(meany, meanx, deltaAz->getValue(), azimuthEncodings, 0, thresh);

    return send(output);
}

// Go up to maxDistance from start.  Stop at bound.
int
SegmentStats::findRangeEnd(size_t az, size_t start, int maxDistance, int bound, VideoT threshold)
{
    int dir = 1;
    if (maxDistance < 0) dir = -1;
    int stop = start + maxDistance;
    int shortStop = stop;
    if (dir * stop > dir * bound) shortStop = bound;

    double maxDrop = maxRangeDrop->getValue();

    VideoT p0 = buffer.get(az, start);
    int range;
    for (range = start; range != shortStop; range += dir) {
        // check
        VideoT p = buffer.get(az, range);
        if (p < threshold || p < maxDrop * p0) return range - start;
        p0 = p;
    }

    return range - start;
}

// Go up to maxDistance from start. If bound is reached, continue from restart.
int
SegmentStats::findAzEnd(size_t range, size_t start, int maxDistance, int bound, size_t restart, VideoT threshold)
{
    int dir = 1;
    if (maxDistance < 0) dir = -1;

    int stop = start + maxDistance;
    int shortStop = stop;
    if (dir * stop > dir * bound) shortStop = bound;

    double maxDrop = maxAzDrop->getValue();

    VideoT p0 = buffer.get(start, range);
    for (int az = start; az != shortStop; az += dir) {
        // check
        VideoT p = buffer.get(az, range);
        if (p < threshold || p < maxDrop * p0) return az - start;
        p0 = p;
    }

    if (shortStop != stop) {
        int newStop = stop - shortStop;

        for (int az = restart; az != newStop; az += dir) {
            // check
            VideoT p = buffer.get(az, range);
            if (p < threshold || p < maxDrop * p0) return (az - restart) + (shortStop - start);
            p0 = p;
        }
    }

    return maxDistance;
}

bool
SegmentStats::processVideo(Messages::Video::Ref msg)
{
    msg->resize(buffer.getDataCols(), 0);

    vsip::Dense<1, VideoT> mMsg(vsip::Domain<1>(buffer.getDataCols()), &msg[0]);
    vsip::Vector<VideoT> vMsg(mMsg);
    mMsg.admit(true);

    currentRow = msg->getShaftEncoding();
    row.setRow(currentRow);
    row.v = vMsg;

    mMsg.release(false);

    return send(msg);
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
SegmentStatsMake(Controller& controller, Logger::Log& log)
{
    return new SegmentStats(controller, log);
}
