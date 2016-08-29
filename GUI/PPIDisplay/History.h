#ifndef SIDECAR_GUI_PPIDISPLAY_HISTORY_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_HISTORY_H

#include <vector>

#include "QtCore/QList"
#include "QtCore/QObject"

#include "Messages/BinaryVideo.h"
#include "Messages/BugPlot.h"
#include "Messages/Extraction.h"
#include "Messages/TSPI.h"
#include "Messages/Video.h"

#include "GUI/TargetPlot.h"

namespace Logger { class Log; }
namespace Utils { class BeamWidthFilter; }

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

class HistorySettings;

/** Container for collections of SideCar messages, grouped by revolution. As messages come in, they are added to
    live and pending Entry objects. When a complete revolution is detected, the pending Entry is placed at the
    front of the retained Entry queue, thus becoming the most-recent historical entry. The oldest Entry object
    is then popped off the back of the queue, keeping the queue a constant size, and the popped off Entry object
    becomes the new pending object.
*/
class History : public QObject
{
    Q_OBJECT
    using Super = QObject;
public:

    using MessageVector = std::vector<Messages::PRIMessage::Ref>;

    /** Log device to use for History messages

        \return Logger::Log reference
    */
    static Logger::Log& Log();

    /** Collection of Video & Extraction objects for one revolution of the radar. The message data is stored in
	std::vector objects used as a circular-buffer.
    */
    class Entry {
    public:

	/** Constructor.
         */
	Entry(size_t videoCapacity, size_t binaryCapacity);

	Messages::Video::DatumType getVideoValue(double azimuth, double range,
                                                 bool& isValid) const;

	Messages::BinaryVideo::DatumType getBinaryValue(double azimuth,
                                                        double range,
                                                        bool& isValid) const;

	/** Determine if there is no data. Note that we only check for Video, since we must have them in order
	    to do any rotation.

	    \return true if empty
	*/
	bool hasVideo() const { return ! video_.empty(); }

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
	const TargetPlotList& getExtractions() const
	    { return extractions_; }

	/** Obtain a read-only reference to the RangeTruth entries.

	    \return TargetPlotList reference
	*/
	const TargetPlotListList& getRangeTruths() const
	    { return rangeTruths_; }

	/** Obtain a read-only reference to the BugPlot entries.

	    \return TargetPlotList reference
	*/
	const TargetPlotList& getBugPlots() const
	    { return bugPlots_; }

	/** Remove all entries from the container.
         */
	void clearAll();

	void clearVideo();

	void clearBinary();

	void clearExtractions();

	void clearRangeTruths();

	void clearBugPlots();

    private:

	/** Add a Video object to the container. Remove any entries that appear to belong to a previous rotation
	    based on the message's azimuth value.

	    \param msg the Video message to add
	*/
	bool addVideo(const Messages::Video::Ref& msg);

	/** Add a BinaryVideo object to the container. Remove any entries that appear to belong to a previous
	    rotation based on the message's azimuth value.

	    \param msg to BinaryVideo message to add
	*/
	void addBinary(const Messages::BinaryVideo::Ref& msg);

	/** Add an Extraction data point to the container.

	    \param msg Extractions value to add
	*/
	void addExtractions(const Messages::Extractions::Ref& msg);

	/** Add an TSPI entry to the container.

	    \param msg TSPI value to add
	*/
	void addRangeTruth(const Messages::TSPI::Ref& msg);

	/** Add a bug plot data point to the container.

	    \param msg BugPlot value to add
	*/
	void addBugPlot(const Messages::BugPlot::Ref& msg);

	void pruneExtractions();

	void pruneRangeTruths();

	void pruneBugPlots();

	void freeze();

	Messages::PRIMessage::Ref findByAzimuth(const MessageVector& data,
                                                double azimuth) const;

	MessageVector video_;
	size_t lastVideoSlot_;
	MessageVector binary_;
	size_t lastBinarySlot_;
	TargetPlotList extractions_;
	TargetPlotListList rangeTruths_;
	TargetPlotList bugPlots_;

	static int extractionsLifeTime_;
	static int rangeTruthsLifeTime_;
	static int rangeTruthsMaxTrailLength_;
	static int bugPlotsLifeTime_;

	friend class History;	///< Grant access to push_back() and clear()
    };

    /** Constructor.

        \param retentionSize number of revolutions to retain

        \param enabled whether retention is enabled
    */
    History(QObject* parent, size_t retentionSize, bool enabled);

    Messages::Video::DatumType getVideoValue(double azimuth, double range,
                                             bool& isValid) const
	{ return viewedEntry_->getVideoValue(azimuth, range, isValid); }

    Messages::BinaryVideo::DatumType getBinaryValue(double azimuth,
                                                    double range,
                                                    bool& isValid) const
	{ return viewedEntry_->getBinaryValue(azimuth, range, isValid); }

    /** Determine if retention is enabled.

        \return true if so
    */
    bool isEnabled() const { return enabled_; }
    
    /** Obtain the number of revolutions retained

        \return retention count
    */
    int getRetentionSize() const { return retentionSize_; }

    int getEntryAge() const { return viewedAge_; }

    int getRetentionCount() const { return retained_.size(); }

    /** Determine if the user is viewing live data

        \return true if so
    */
    bool showingLiveEntry() const { return viewedEntry_ == &liveEntry_; }

    /** Determine if the user is viewing historical data

        \return true if so
    */
    bool showingPastEntry() const { return ! showingLiveEntry(); }

    /** Obtain a reference to the Entry being viewed by the user.

        \return Entry reference
    */
    const Entry& getViewedEntry() const { return *viewedEntry_; }

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

    /** Add a RangeTruth object to the live view, and possibly to a retained view if enabled.

        \param msg message to add
    */
    void addRangeTruth(const Messages::TSPI::Ref& msg);

    /** Add one TargetPlot object to the live view, and possibly to a retained view if enabled. This is from a
        user clicking on the screen.

        \param msg message to add
    */
    void addBugPlot(const Messages::BugPlot::Ref& msg);

public slots:

    /** Change whether retention is enabled

        \param enabled new retention state
    */
    void setEnabled(bool enabled);

    /** Change the number of revolutions retained

        \param retentionSize new value
    */
    void setRetentionSize(int retention);

    /** Change the viewing state so that live data is visible
     */
    void showLiveEntry();

    /** Change the viewing state so that the indicated past revolution is visible.

        \param offset revolution in the past to view
    */
    void showEntry(int slot);

    void setExtractionsLifeTime(int msecs)
	{ Entry::extractionsLifeTime_ = msecs; }

    void setRangeTruthsLifeTime(int msecs)
	{ Entry::rangeTruthsLifeTime_ = msecs; }

    void setRangeTruthsMaxTrailLength(int length)
	{ Entry::rangeTruthsMaxTrailLength_ = length; }

    void setBugPlotsLifeTime(int msecs)
	{ Entry::bugPlotsLifeTime_ = msecs; }

    /** Erase all of the data in the live and pending views. Existing historical data is NOT erased.
     */
    void clearAll();

    void clearVideo();

    void clearBinary();

    void clearExtractions();

    void clearRangeTruths();

    void clearBugPlots();

signals:

    void currentViewChanged(int age);

    void currentViewAged(int age);

    void retainedCountChanged(int retained);

private:
    using EntryVector = std::vector<Entry*>;

    EntryVector retained_;	///< Collection of past revolution data
    size_t retentionSize_;	///< Maximum number of entries - 1 to keep
    size_t oldestRetained_;	///< Index of the oldest retained entry

    Entry liveEntry_;		///< Entry with live data
    Entry* pendingEntry_;	///< Entry with pending retained data

    Entry* viewedEntry_;	///< 
    size_t viewedAge_;

    bool viewedEntryIsRetained_; ///< True if viewedEntry is in retained_
    bool enabled_;		 ///< True if retention is enabled
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
