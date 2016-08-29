#ifndef SIDECAR_GUI_PLAYBACK_EMITTER_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_EMITTER_H

#include "QtCore/QThread"

#include "IO/Readers.h"
#include "Messages/Header.h"
#include "Time/TimeStamp.h"

class ACE_Message_Block;
class QFileInfo;

namespace Logger { class Log; }

namespace SideCar {
namespace IO { class TimeIndex; }
namespace GUI {

class MessageWriter;

namespace Playback {

class Clock;
class MainWindow;

/** Message emitter for the Playback application. Takes an existing recording file and a IO::TimeIndex file
    derived from it, and emits records from the file with time sequencing similar to the the original recording,
    only transposed to a current time frame.

    The Emitter processing happens in a separate thread in the run() method. The main thread controls thread execution
    via the start() and stop() methods.
*/
class Emitter : public QThread
{
    Q_OBJECT
    using Super = QThread;
public:

    /** Log device for instances of this class.

        \return log device
    */
    static Logger::Log& Log();

    /** Constructor. Initializes a new Emitter object.

        \param mainWindow the MainWindow parent of the Emitter

        \param fileInfo QFileInfo object referring to the data file to emit

        \param emitting true if the emitter will emit when start() is called
    */
    Emitter(MainWindow* mainWindow, const QFileInfo& fileInfo, int row, bool emitting);

    /** Destructor. Makes sure thread is stopped.
     */
    ~Emitter();

    bool isValid() const { return valid_; }

    void load(const QFileInfo& fileInfo);

    /** Obtain the name of the recording file being emitted.
        
        \return file name
    */
    const QString& getName() const { return name_; }

    int getPort() const;

    /** Obtain the time of the first record in the file. This is in the recording's frame of reference.
        
	\return Time::TimeStamp value in clock time
    */
    const Time::TimeStamp& getStartTime() const { return startTime_; }

    /** Obtain the time of the last record in the file. This is in the recording's frame of reference.
        
        \return Time::TimeStamp value in clock time
    */
    const Time::TimeStamp& getEndTime() const { return endTime_; }

    /** Obtain the duration of the recording file we represent.
        
        \return Time::TimeStamp value containing getEndTime() - getStartTime()
    */
    Time::TimeStamp getDuration() const { return endTime_ - startTime_; }

    /** Determine if the emitter is enabled to run.
        
        \return true if so
    */
    bool getEmitting() const { return emitting_; }

    double getLoadPercentage() const { return loadPercentage_; }

    size_t getSubscriberCount() const { return subscriberCount_; }

    /** Set whether the emitter should run when start() is invoked.
        
        \param emitting new value
    */
    void setEmitting(bool emitting);

    QString getFormattedStartTime() const;

    QString getFormattedEndTime() const;

    QString getFormattedDuration() const;

    int getRow() const { return row_; }

    /** Create a new publisher for the data stream.
     */
    void makeWriter(bool restart = false);

signals:

    void loadPercentageUpdate(int row);

    void subscriberCountChanged(int row);

private slots:

    /** Signal handler for multicast address changes by the user in the MainWindow window. If an emitter is
        running, this will stop it and restart it after the change is made.

        \param address new address to use
    */
    void setAddress(const QString& address);

    /** Signal handler for suffix changes by the user in the MainWindow window. If an emitter is running, this
        will stop it and restart it after the change is made.
        
        \param suffix new suffix to use
    */
    void setSuffix(const QString& suffix);

    /** Change the current position in the recording file such that it points to the first record with a
        timestamp greater than or equal to the given value. Uses the held IO::TimeIndex that provides fast
        binary searching of time values to get near to the desired record; the position() method then finishes
        with a (hopefully small) linear search.
        
        \param when the value to search for
    */
    void setPlaybackClockStart(const Time::TimeStamp& when);

    /** Start the emitter if it is enabled. Playback will begin at the current pending record.
     */
    void start();

    /** Stop the emitter if it is running. Does not return until the thread has been reaped via QThread::wait().
     */
    void stop();

    void writerSubscriberCountChanged(size_t value);

private:

    /** Method the runs in a separate thread. Performs the actual emitting of data.
     */
    void run();
    
    /** Jump to the given file offset and attempt to read in the message record there. Updates pending_ and
        pendingTime_ attributes.
        
        \param offset file offset to move to

	\param when time stamp to search for
    */
    void repositionAndFetch(off_t offset, const Time::TimeStamp& when);

    void indexMakerUpdate(double percentageComplete);

    Clock* clock_;
    QString name_;
    QString address_;
    QString suffix_;
    const Messages::MetaTypeInfo* metaTypeInfo_;
    IO::FileReader reader_;
    IO::TimeIndex* timeIndex_;
    MessageWriter* writer_;
    ACE_Message_Block* pending_;
    Time::TimeStamp startTime_;
    Time::TimeStamp endTime_;
    Time::TimeStamp pendingTime_;
    double loadPercentage_;
    size_t subscriberCount_;
    int row_;
    bool emitting_;
    bool valid_;
    volatile bool running_;
};

} // end namespace Playback
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
