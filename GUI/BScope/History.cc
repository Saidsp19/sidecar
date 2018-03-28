#include <algorithm>
#include <functional>
#include <iterator>

#include "GUI/LogUtils.h"
#include "Utils/Utils.h"

#include "History.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;
using namespace SideCar::Messages;

Logger::Log&
History::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.History");
    return log_;
}

History::History(QObject* parent) :
    QObject(parent), video_(), lastVideoSlot_(-1), binary_(), lastBinarySlot_(-1), extractions_(), rangeTruths_(),
    bugPlots_(), extractionsLifeTime_(30 * 1000), rangeTruthsLifeTime_(30 * 1000), rangeTruthsMaxTrailLength_(10),
    bugPlotsLifeTime_(30 * 1000), bugPlotsCounter_(0)
{
    video_.reserve(1000);
    binary_.reserve(10000);
    clear();
}

PRIMessage::Ref
History::findByAzimuth(const MessageVector& data, double azimuth) const
{
    static const int kMinDelta = 10;

    if (data.empty()) return PRIMessage::Ref();

    // Obtain shaft encoding for the given azimuth value.
    //
    int wanted = static_cast<int>(::rint(azimuth * (RadarConfig::GetShaftEncodingMax() + 1) / (M_PI * 2.0)));

    MessageVector::const_iterator low = data.begin();
    int lowShaft = (*low)->getShaftEncoding();
    if (wanted < lowShaft) wanted += (RadarConfig::GetShaftEncodingMax() + 1);

    MessageVector::const_iterator high = data.end() - 1;
    int highShaft = (*high)->getShaftEncoding();
    if (highShaft < wanted) highShaft += (RadarConfig::GetShaftEncodingMax() + 1);

    while (low < high) {
        MessageVector::const_iterator mid = low + (high - low) / 2;
        const PRIMessage::Ref& msg(*mid);
        int midShaft = msg->getShaftEncoding();

        if (midShaft < lowShaft) midShaft += (RadarConfig::GetShaftEncodingMax() + 1);

        if (wanted == midShaft) {
            return msg;
            break;
        }

        if (mid == low) {
            if (wanted == highShaft) return (*high);

            int deltaLow = wanted - lowShaft;
            int deltaHigh = highShaft - wanted;
            if (deltaLow < deltaHigh) {
                if (deltaLow < kMinDelta) return *low;
            } else {
                if (deltaHigh < kMinDelta) return *high;
            }
            break;
        }

        if (wanted < midShaft) {
            high = mid;
            highShaft = midShaft;
        } else {
            low = mid;
            lowShaft = midShaft;
        }
    }

    return PRIMessage::Ref();
}

Video::DatumType
History::getVideoValue(double azimuth, double range, bool& isValid) const
{
    PRIMessage::Ref found(findByAzimuth(video_, azimuth));
    if (!found) {
        isValid = false;
        return Video::DatumType();
    }

    size_t wantedRangeGate = ::rint((range - found->getRangeMin()) / found->getRangeFactor());
    if (wantedRangeGate >= found->size()) {
        isValid = false;
        return Video::DatumType();
    }

    const Video::Ref video = boost::dynamic_pointer_cast<Video>(found);
    isValid = true;
    return video[wantedRangeGate];
}

BinaryVideo::DatumType
History::getBinaryValue(double azimuth, double range, bool& isValid) const
{
    PRIMessage::Ref found(findByAzimuth(binary_, azimuth));
    if (!found) {
        isValid = false;
        return Video::DatumType();
    }

    size_t wantedRangeGate = ::rint((range - found->getRangeMin()) / found->getRangeFactor());
    if (wantedRangeGate >= found->size()) {
        isValid = false;
        return Video::DatumType();
    }

    const BinaryVideo::Ref binary = boost::dynamic_pointer_cast<BinaryVideo>(found);
    isValid = true;
    return binary[wantedRangeGate];
}

void
History::addVideo(const Messages::Video::Ref& msg)
{
    if (video_.empty()) {
        video_.push_back(msg);
        lastVideoSlot_ = 0;
        return;
    }

    int index = lastVideoSlot_;
    if (index == -1) index = video_.size() - 1;
    Messages::PRIMessage::Ref last = video_[index];
    if (msg->getShaftEncoding() == last->getShaftEncoding()) {
        video_[index] = msg;
        return;
    }

    ++lastVideoSlot_;
    if (size_t(lastVideoSlot_) == video_.size()) {
        video_.push_back(msg);
    } else {
        video_[lastVideoSlot_] = msg;
    }

    pruneExtractions();
    pruneRangeTruths();
    pruneBugPlots();
}

void
History::pruneExtractions()
{
    // Prune out extractions that have lived a full life.
    //
    TargetPlotList::iterator pos = extractions_.begin();
    TargetPlotList::iterator end = extractions_.end();
    while (pos != end && pos->getAge() >= extractionsLifeTime_) ++pos;
    extractions_.erase(extractions_.begin(), pos);
}

void
History::pruneRangeTruths()
{
    // Prune out range truths that have lived a full life.
    //
    TargetPlotListList::iterator pos = rangeTruths_.begin();
    TargetPlotListList::iterator end = rangeTruths_.end();
    while (pos != end && pos->front().getAge() >= rangeTruthsLifeTime_) ++pos;
    rangeTruths_.erase(rangeTruths_.begin(), pos);
}

void
History::pruneBugPlots()
{
    // Prune out user bug plots that have lived a full life.
    //
    TargetPlotList::iterator pos = bugPlots_.begin();
    TargetPlotList::iterator end = bugPlots_.end();
    while (pos != end && pos->getAge() >= bugPlotsLifeTime_) ++pos;
    bugPlots_.erase(bugPlots_.begin(), pos);
}

void
History::addBinary(const Messages::BinaryVideo::Ref& msg)
{
    if (video_.empty()) return;

    if (binary_.empty()) {
        binary_.push_back(msg);
        lastBinarySlot_ = 0;
        return;
    }

    int index = lastBinarySlot_;
    if (index == -1) index = binary_.size() - 1;
    Messages::PRIMessage::Ref last = binary_[index];
    if (msg->getShaftEncoding() == last->getShaftEncoding()) {
        binary_[index] = msg;
        return;
    }

    ++lastBinarySlot_;
    if (size_t(lastBinarySlot_) == binary_.size()) {
        binary_.push_back(msg);
    } else {
        binary_[lastBinarySlot_] = msg;
    }
}

void
History::addExtractions(const Messages::Extractions::Ref& msg)
{
    QString tag(QString("%1/%2").arg(msg->getMessageSequenceNumber()));
    for (size_t index = 0; index < msg->size(); ++index) {
        extractions_.push_back(TargetPlot(msg[index], tag.arg(index)));
    }
}

void
History::addRangeTruth(const Messages::TSPI::Ref& msg)
{
    QString tag = QString::fromStdString(msg->getTag());

    TargetPlotListList::iterator pos = rangeTruths_.begin();
    TargetPlotListList::iterator end = rangeTruths_.end();

    while (pos != end) {
        TargetPlotList& plots(*pos);
        if (plots.front().getTag() == tag) {
            if (msg->isDropping()) {
                rangeTruths_.erase(pos);
                break;
            }

            plots.push_front(TargetPlot(*msg));
            while (plots.size() > rangeTruthsMaxTrailLength_) { plots.pop_back(); }

            return;
        }
        ++pos;
    }

    if (!msg->isDropping()) {
        TargetPlotList plots;
        plots.push_back(TargetPlot(*msg));
        rangeTruths_.push_back(plots);
    }
}

void
History::addBugPlot(const Messages::BugPlot::Ref& msg)
{
    bugPlots_.push_back(TargetPlot(*msg));
}

void
History::clear()
{
    clearVideo();
    clearBinary();
    clearExtractions();
    clearRangeTruths();
    clearBugPlots();
}

void
History::clearVideo()
{
    video_.clear();
    lastVideoSlot_ = 0;
}

void
History::clearBinary()
{
    binary_.clear();
    lastBinarySlot_ = 0;
}

void
History::clearExtractions()
{
    extractions_.clear();
}

void
History::clearRangeTruths()
{
    rangeTruths_.clear();
}

void
History::clearBugPlots()
{
    bugPlots_.clear();
}

void
History::wrap()
{
    lastVideoSlot_ = -1;
    lastBinarySlot_ = -1;
}
