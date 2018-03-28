#ifndef SIDECAR_GUI_PLAYBACK_CLOCK_H // -*- C++ -*-
#define SIDECAR_GUI_PLAYBACK_CLOCK_H

#include "QtCore/QObject"
#include "QtCore/QTimer"

#include "Time/TimeStamp.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace Playback {

/** Playback clock for the Playback application. There are two time frames of reference in use in the Playback
    application. The first is the time frame of the recorded data. Timestamps in the recorded data are UTC time
    values that indicate when the data was recorded. One can subtract one of these values from another to obtain
    their delta in seconds or microseconds. Note that these values not only indicate the clock time of the
    recording, but they also indicate the date on which the recording took place.

    The second time frame refers to the time when playback starts, and the
    number of seconds that have elapsed during data playback. The
    getPlaybackClock() and getWallClockDurationUntil() methods manage the two
    time frames in order to provide a fairly accurate reproduction of the
    message emission that was seen on the original system when the data was
    recorded. The Clock class supports playback rates that are faster and
    slower than real time. Calling setWallClockRate() with a value greater than
    one will result in playback that is faster than real time; likewise, giving
    setWallClockRate() a non-zero value less than one will result in playback
    that is slower than real time.
*/
class Clock : public QObject {
    Q_OBJECT
public:
    static Logger::Log& Log();

    /** Constructor.

        \param parent object that acquires ownership of this one

        \param tickBeat number of milliseconds between tick() signal emissions
    */
    Clock(QObject* parent, int tickBeat, double wallClockRate);

    /** Obtain the playback clock value for the current wall clock time.

        \return
    */
    Time::TimeStamp getPlaybackClock();

    /** Obtain the number of seconds in wall clock time before the event with the given will occur. Accounts for
        non-unity playback rate, so the value is valid for use with sleep() or usleep().

        \param when the time to check

        \return wall time duration until event
    */
    Time::TimeStamp getWallClockDurationUntil(const Time::TimeStamp& when);

    /** Set the time frame of the playback clock. A playback clock with an elapsed time greater than the set
        duration value will stop, unless looping is enabled in which case the clock will keep running and just
        jump back to the start time after the entire duration has elapsed.

        \param start the start of the time frame

        \param start the end of the time frame
    */
    void setClockRange(const Time::TimeStamp& start, const Time::TimeStamp& end);

    /** Set the periodic tick rate.

        \param msecs milliseconds between tick() signal emissions.
    */
    void setTickRate(int msecs) { ticker_.setInterval(msecs); }

    /** Set the wall clock rate. Values must be greater than zero. Values greater than one will result in a
        playback clock that runs faster than real-time, while values less than one will give one that runs
        slower than real-time. NOTE: it is OK to call this while the clock is running

        \param rate wall clock rate
    */
    void setWallClockRate(double rate);

    /** Determine if the clock is currently running.

        \return true if so
    */
    bool isRunning() const { return ticker_.isActive(); }

    /** Obtain a formatted QString for a given absolute TimeStamp value. The given value is reduced by the
        midnight_ value.

        \param when the time to format

        \return the formatted value in HH:MM:SS format
    */
    QString formatTimeStamp(const Time::TimeStamp& when);

    /** Obtain a formatted QString for a given duration TimeStamp value value.

        \param duration value to format

        \return the formatted value in HH:MM:SS format
    */
    QString formatDuration(const Time::TimeStamp& duration);

public slots:

    /** Start the playback clock. Starts an internal QTimer to perform ticking, and emits the started() signal.
     */
    void start();

    /** Stop the playback clock. Stops the internal QTimer used to perform ticking, and emits the stopped()
        signal.
    */
    void stop();

    /** Change the playback clock so that it will start at a new time.

        \param playbackClockStart
    */
    void setPlaybackClockStart(const Time::TimeStamp& playbackClockStart);

private slots:

    /** Event handler for when the clock ticks. Invoked by the internal QTimer when it times out.
     */
    void doTick();

signals:

    /** Notification sent out when the clock ticks. Emits the current playback clock time, and the amount of
        time that has elapsed in the playback.

        \param playbackTime current playback clock time

        \param elapsed playback duration
    */
    void tick(const Time::TimeStamp& playbackTime, const Time::TimeStamp& elapsed);

    /** Notification sent out when the playback clock has started.
     */
    void started();

    /** Notification sent out when the playback clock has stopped.
     */
    void stopped();

    /** Notification sent out when he start time of the playback clock has changed.

        \param playbackClockStart
    */
    void playbackClockStartChanged(const Time::TimeStamp& playbackClockStart);

private:
    QTimer ticker_;                      ///< Timer that updates our playback clock time
    double wallClockRate_;               ///< How fast/slow wall clock runs vs. playback
    double playbackClockRate_;           ///< Inverse of wallClockRate_
    Time::TimeStamp minTime_;            ///< First time stamp in all of the data files
    Time::TimeStamp maxTime_;            ///< Last time stamp in all of the data files
    Time::TimeStamp playbackClockStart_; ///< Playback clock when playback began
    Time::TimeStamp wallClockStart_;     ///< System clock when playback began
};

} // end namespace Playback
} // end namespace GUI
} // end namespace SideCar

#endif
