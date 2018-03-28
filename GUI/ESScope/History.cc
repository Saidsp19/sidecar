#include <algorithm>
#include <functional>
#include <iterator>

#include "GUI/BoolSetting.h"
#include "GUI/LogUtils.h"
#include "GUI/MessageList.h"
#include "GUI/RangeTruthsImaging.h"
#include "GUI/TargetPlotImaging.h"
#include "Utils/Utils.h"

#include "History.h"
#include "RadarSettings.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

Logger::Log&
History::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.History");
    return log_;
}

History::History(QObject* parent, RadarSettings* radarSettings, TargetPlotImaging* extractionsImaging,
                 RangeTruthsImaging* rangeTruthsImaging, TargetPlotImaging* bugPlotsImaging, BoolSetting* allynHack) :
    QObject(parent),
    radarSettings_(radarSettings), alphaBetaData_(), alphaRangeData_(), extractions_(), rangeTruths_(), bugPlots_(),
    allynHack_(allynHack), extractionsLifeTime_(extractionsImaging->getLifeTime()),
    rangeTruthsLifeTime_(rangeTruthsImaging->getLifeTime()),
    rangeTruthsMaxTrailLength_(rangeTruthsImaging->getTrailSize()), bugPlotsLifeTime_(bugPlotsImaging->getLifeTime()),
    scanCounter_(0), lastAlphaIndex_(-1)
{
    connect(radarSettings, SIGNAL(scansChanged(int, int, int)), SLOT(scansChanged(int, int, int)));
    scansChanged(radarSettings->getAlphaScans(), radarSettings->getBetaScans(), radarSettings->getRangeScans());
    connect(extractionsImaging, SIGNAL(lifeTimeChanged(int)), SLOT(extractionsLifeTimeChanged(int)));
    connect(rangeTruthsImaging, SIGNAL(lifeTimeChanged(int)), SLOT(rangeTruthsLifeTimeChanged(int)));
    connect(rangeTruthsImaging, SIGNAL(trailSizeChanged(int)), SLOT(rangeTruthsMaxTrailLengthChanged(int)));
    connect(bugPlotsImaging, SIGNAL(lifeTimeChanged(int)), SLOT(bugPlotsLifeTimeChanged(int)));
}

void
History::scansChanged(int alphaScans, int betaScans, int rangeScans)
{
    alphaBetaData_.clear();
    alphaBetaData_.resize(alphaScans * betaScans);
    alphaRangeData_.clear();
    alphaRangeData_.resize(alphaScans);
    for (int index = 0; index < alphaScans; ++index) alphaRangeData_[index].resize(rangeScans);
}

void
History::pruneTargets()
{
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
History::clearAll()
{
    clearVideo();
    clearExtractions();
    clearRangeTruths();
    clearBugPlots();
}

void
History::clearVideo()
{
    for (size_t index = 0; index < alphaBetaData_.size(); ++index) { alphaBetaData_[index].reset(); }

    for (size_t index = 0; index < alphaRangeData_.size(); ++index) {
        alphaRangeData_[index].clear();
        alphaRangeData_[index].resize(radarSettings_->getRangeScans());
    }

    lastAlphaIndex_ = -1;
    scanCounter_ = 0;
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
History::processVideo(const MessageList& data)
{
    static Logger::ProcLog log("processVideo", Log());
    LOGINFO << "data.size: " << data.size() << std::endl;

    Messages::Video::Ref info;
    AlphaIndices alphaIndices;

    int betaScans = radarSettings_->getBetaScans();
    int firstSample = radarSettings_->getFirstSample();
    int lastSample = radarSettings_->getLastSample();

    for (size_t index = 0; index < data.size(); ++index) {
        Messages::Video::Ref msg(boost::dynamic_pointer_cast<Messages::Video>(data[index]));
        LOGDEBUG << msg->getSequenceCounter() << std::endl;

        // !!! HACK !!!
        //
        if (!allynHack_->getValue() && msg->getRIUInfo().prfEncoding == 0) continue;

        info = msg;

        // Use the latest range information.
        //
        radarSettings_->setRangeScaling(msg->getRangeMin(), msg->getRangeFactor());

        int alphaIndex = radarSettings_->getAlphaIndex(msg);
        if (alphaIndex == -1) {
            LOGERROR << "invalid alpha value: " << radarSettings_->getAlpha(msg) << " - skipping message "
                     << msg->getSequenceCounter() << std::endl;
            continue;
        }

        // Detect when we've reached the end of the scan in alpha space.
        //
        if (alphaIndex < lastAlphaIndex_) {
            LOGINFO << "seq: " << msg->getSequenceCounter() << " lastAlphaIndex: " << lastAlphaIndex_
                    << " alphaIndex: " << alphaIndex << std::endl;
            ++scanCounter_;

            // If we have any changes, go ahead and emit them now. NOTE: this behavior is required by the
            // AlphaBetaWidget and AlphaRangeWidget classes so that they can transfer large streams of color
            // values to the graphics card in one shot.
            //
            if (!alphaIndices.empty()) {
                emit alphasChanged(alphaIndices);
                alphaIndices.clear();
            }
        }

        lastAlphaIndex_ = alphaIndex;

        // Add the alpha index if not already present.
        //
        if (alphaIndices.empty() || alphaIndices.back() != alphaIndex) alphaIndices.push_back(alphaIndex);

        int betaIndex = radarSettings_->getBetaIndex(msg);
        if (betaIndex == -1) {
            LOGERROR << "invalid beta value: " << radarSettings_->getBeta(msg) << " - skipping message "
                     << msg->getSequenceCounter() << std::endl;
            continue;
        }

        // Find the largest value in the sample range
        //
        size_t alphaBetaIndex = alphaIndex * betaScans + betaIndex;
        Messages::Video::const_iterator pos = msg->begin();
        Messages::Video::const_iterator end = msg->end();
        Messages::Video::const_iterator first = pos + firstSample;
        Messages::Video::const_iterator last = pos + lastSample;
        int value = DataContainer::GetMinValue();
        if (first < end) {
            if (last >= end) last = end;
            value = *std::max_element(first, last);
        }

        // Record the found value if larger than the previous or if we are in a new scan.
        //
        if (value > alphaBetaData_[alphaBetaIndex].value || scanCounter_ != alphaBetaData_[alphaBetaIndex].scan) {
            alphaBetaData_[alphaBetaIndex].value = value;
            alphaBetaData_[alphaBetaIndex].scan = scanCounter_;
        }

        // Update the data for the AlphaRangeWidget.
        //
        alphaRangeData_[alphaIndex].update(msg, radarSettings_, scanCounter_);
    }

    // Notify the views of changes.
    //
    if (!alphaIndices.empty()) emit alphasChanged(alphaIndices);

    // Notify others of the last message processed.
    //
    if (info) emit currentMessage(info);

    pruneTargets();
}

void
History::processExtractions(const MessageList& data)
{
    static Logger::ProcLog log("processExtractions", Log());
    LOGINFO << std::endl;

    for (size_t index = 0; index < data.size(); ++index) {
        Messages::Extractions::Ref msg(boost::dynamic_pointer_cast<Messages::Extractions>(data[index]));

        QString tag(QString("%1/%2").arg(msg->getMessageSequenceNumber()));
        for (size_t index = 0; index < msg->size(); ++index) {
            extractions_.push_back(TargetPlot(msg[index], tag.arg(index)));
        }
    }
}

void
History::processRangeTruths(const MessageList& data)
{
    static Logger::ProcLog log("processRangeTruths", Log());
    LOGINFO << std::endl;

    for (size_t index = 0; index < data.size(); ++index) {
        Messages::TSPI::Ref msg(boost::dynamic_pointer_cast<Messages::TSPI>(data[index]));
        LOGINFO << "range: " << msg->getRange() << " azimuth: " << msg->getAzimuth()
                << " elevation: " << msg->getElevation() << std::endl;
        addRangeTruth(msg);
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
                return;
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
History::processBugPlots(const MessageList& data)
{
    static Logger::ProcLog log("processBugPlots", Log());
    LOGINFO << std::endl;

    for (size_t index = 0; index < data.size(); ++index) {
        Messages::BugPlot::Ref msg(boost::dynamic_pointer_cast<Messages::BugPlot>(data[index]));
        addBugPlot(msg);
    }
}

void
History::addBugPlot(const Messages::BugPlot::Ref& msg)
{
    bugPlots_.push_back(TargetPlot(*msg));
}
