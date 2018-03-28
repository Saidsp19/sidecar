#ifndef SIDECAR_GUI_ESSCOPE_HISTORY_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_HISTORY_H

#include "QtCore/QObject"

#include "Messages/BugPlot.h"
#include "Messages/Extraction.h"
#include "Messages/TSPI.h"
#include "Messages/Video.h"

#include "AlphaRangeColumn.h"
#include "TargetPlot.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

class BoolSetting;
class MessageList;
class RangeTruthsImaging;
class TargetPlotImaging;

namespace ESScope {

class RadarSettings;

using AlphaIndices = std::vector<int>;

class History : public QObject {
    Q_OBJECT
    using Super = QObject;

public:
    static Logger::Log& Log();

    struct AlphaBetaInfo {
        AlphaBetaInfo() : value(DataContainer::GetMinValue()), scan(-1) {}
        void reset() { *this = AlphaBetaInfo(); }
        int value;
        int scan;
    };

    using AlphaRangeData = std::vector<AlphaRangeColumn>;
    using AlphaBetaData = std::vector<AlphaBetaInfo>;

    /** Constructor.
     */
    History(QObject* parent, RadarSettings* radarSettings, TargetPlotImaging* extractionsImaging,
            RangeTruthsImaging* rangeTruthsImaging, TargetPlotImaging* bugPlotsImaging, BoolSetting* allynHack);

    int getAlphaBetaValue(size_t index) const { return alphaBetaData_[index].value; }

    /**

        \param alphaIndex

        \return
    */
    const AlphaRangeColumn& getAlphaRangeData(size_t alphaIndex) const { return alphaRangeData_[alphaIndex]; }

    /** Obtain a read-only reference to the Extraction message collection

        \return TargetPlotList reference
    */
    const TargetPlotList& getExtractions() const { return extractions_; }

    /** Obtain a read-only reference to the TSPI message collection

        \return TargetPlotList reference
    */
    const TargetPlotListList& getRangeTruths() const { return rangeTruths_; }

    /** Obtain a read-only reference to the user bug plot entries.

        \return TargetPlotList reference
    */
    const TargetPlotList& getBugPlots() const { return bugPlots_; }

    void addBugPlot(const Messages::BugPlot::Ref& msg);

signals:

    void alphasChanged(const AlphaIndices& indices);

    void currentMessage(const Messages::PRIMessage::Ref& msg);

public slots:

    void processVideo(const MessageList& data);

    void processExtractions(const MessageList& data);

    void processRangeTruths(const MessageList& data);

    void processBugPlots(const MessageList& data);

    void pruneTargets();

    void clearAll();

    void clearVideo();

    void clearExtractions();

    void clearRangeTruths();

    void clearBugPlots();

private slots:

    void addRangeTruth(const Messages::TSPI::Ref& msg);

    void scansChanged(int alphaScans, int betaScans, int rangeScans);

    void extractionsLifeTimeChanged(int msecs) { extractionsLifeTime_ = msecs; }

    void rangeTruthsLifeTimeChanged(int msecs) { rangeTruthsLifeTime_ = msecs; }

    void rangeTruthsMaxTrailLengthChanged(int length) { rangeTruthsMaxTrailLength_ = length; }

    void bugPlotsLifeTimeChanged(int msecs) { bugPlotsLifeTime_ = msecs; }

private:
    void pruneExtractions();

    void pruneRangeTruths();

    void pruneBugPlots();

    RadarSettings* radarSettings_;
    AlphaBetaData alphaBetaData_;
    AlphaRangeData alphaRangeData_;
    TargetPlotList extractions_;
    TargetPlotListList rangeTruths_;
    TargetPlotList bugPlots_;
    BoolSetting* allynHack_;
    int extractionsLifeTime_;
    int rangeTruthsLifeTime_;
    int rangeTruthsMaxTrailLength_;
    int bugPlotsLifeTime_;
    int scanCounter_;
    int lastAlphaIndex_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
