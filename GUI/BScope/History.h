#ifndef SIDECAR_GUI_BSCOPE_HISTORY_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_HISTORY_H

#include <vector>

#include "QtCore/QList"
#include "QtCore/QObject"

#include "Messages/BinaryVideo.h"
#include "Messages/BugPlot.h"
#include "Messages/Extraction.h"
#include "Messages/TSPI.h"
#include "Messages/Video.h"

#include "TargetPlot.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace BScope {

/** Container for collections of SideCar messages, grouped by revolution. As messages come in, they are added to
    live and pending Entry objects. When a complete revolution is detected, the pending Entry is placed at the
    front of the retained Entry queue, thus becoming the most-recent historical entry. The oldest Entry object
    is then popped off the back of the queue, keeping the queue a constant size, and the popped off Entry object
    becomes the new pending object.
*/
class History : public QObject {
    Q_OBJECT
public:
    static Logger::Log& Log();

    using MessageVector = std::vector<Messages::PRIMessage::Ref>;

    /** Constructor.

        \param retentionSize number of revolutions to retain

        \param enabled whether retention is enabled
    */
    History(QObject* parent);

    Messages::Video::DatumType getVideoValue(double azimuth, double range, bool& isValid) const;

    Messages::BinaryVideo::DatumType getBinaryValue(double azimuth, double range, bool& isValid) const;

    /** Obtain a read-only reference to the Video collection

        \return MessageVector reference
    */
    const MessageVector& getVideo() const { return video_; }

    /** Obtain a read-only reference to the BinaryVideo collection

        \return MessageVector reference
    */
    const MessageVector& getBinary() const { return binary_; }

    /** Obtain a read-only reference to the Extraction message collection

        \return TargetPlotList reference
    */
    const TargetPlotList& getExtractions() const { return extractions_; }

    /** Obtain a read-only reference to the TSPI message collection

        \return TargetPlotListList reference
    */
    const TargetPlotListList& getRangeTruths() const { return rangeTruths_; }

    /** Obtain a read-only reference to the user bug plot entries.

        \return TargetPlotList reference
    */
    const TargetPlotList& getBugPlots() const { return bugPlots_; }

    /** Add a Video object to the live view, and possibly to a retained view if enabled.

        \param msg message to add
    */
    void addVideo(const Messages::Video::Ref& msg);

    /** Add a BinaryVideo object the the live view. and possibly to a retained view if enabled.

        \param msg message to add
    */
    void addBinary(const Messages::BinaryVideo::Ref& msg);

    /** Add an Extractions object to the live view, and possibly to a retained view if enabled.

        \param msg message to add
    */
    void addExtractions(const Messages::Extractions::Ref& msg);

    /** Add a TSPI object to the live view, and possibly to a retained view if enabled.

        \param msg message to add
    */
    void addRangeTruth(const Messages::TSPI::Ref& msg);

    /** Add one TargetPlot object to the live view, and possibly to a retained view if enabled. This is from a
        user clicking on the screen.

        \param msg bug plot to add
    */
    void addBugPlot(const Messages::BugPlot::Ref& msg);

    void wrap();

    /** Erase all cached data.
     */
    void clear();

    void clearVideo();

    void clearBinary();

    void clearExtractions();

    void clearRangeTruths();

    void clearBugPlots();

public slots:

    void setExtractionsLifeTime(int msecs) { extractionsLifeTime_ = msecs; }

    void setRangeTruthsLifeTime(int msecs) { rangeTruthsLifeTime_ = msecs; }

    void setRangeTruthsMaxTrailLength(int length) { rangeTruthsMaxTrailLength_ = length; }

    void setBugPlotsLifeTime(int msecs) { bugPlotsLifeTime_ = msecs; }

private:
    Messages::PRIMessage::Ref findByAzimuth(const MessageVector& data, double azimuth) const;

    void pruneExtractions();

    void pruneRangeTruths();

    void pruneBugPlots();

    MessageVector video_;
    int lastVideoSlot_;
    MessageVector binary_;
    int lastBinarySlot_;
    TargetPlotList extractions_;
    TargetPlotListList rangeTruths_;
    TargetPlotList bugPlots_;
    int extractionsLifeTime_;
    int rangeTruthsLifeTime_;
    int rangeTruthsMaxTrailLength_;
    int bugPlotsLifeTime_;
    int bugPlotsCounter_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
