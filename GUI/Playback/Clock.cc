
#include "GUI/LogUtils.h"

#include "Clock.h"

using namespace SideCar;
using namespace SideCar::GUI::Playback;

Logger::Log&
Clock::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("playback.Clock");
    return log_;
}

Clock::Clock(QObject* parent, int tickBeat, double wallClockRate)
    : QObject(parent), ticker_(), wallClockRate_(wallClockRate),
      playbackClockRate_(1.0 / wallClockRate)
{
    ticker_.setInterval(tickBeat);
    connect(&ticker_, SIGNAL(timeout()), this, SLOT(doTick()));
}

Time::TimeStamp
Clock::getPlaybackClock()
{
    static Logger::ProcLog log("getPlaybackClock", Log());

    if (! ticker_.isActive()) {
	LOGDEBUG << playbackClockStart_ << std::endl;
	return playbackClockStart_;
    }

    // Calculate the number of seconds that have elapsed in real time.
    //
    Time::TimeStamp elapsed(Time::TimeStamp::Now());
    elapsed -= wallClockStart_;

    // Apply a rate to the elapsed real time to make playback time pass faster or slower.
    //
    if (wallClockRate_ != 1.0)
	elapsed *= wallClockRate_;

    // Add any past elapsed time to the time when playback started to get the current playback clock time.
    //
    elapsed += playbackClockStart_;
    LOGDEBUG << "clock: " << elapsed << std::endl;

    return elapsed;
}

Time::TimeStamp
Clock::getWallClockDurationUntil(const Time::TimeStamp& when)
{
    static Logger::ProcLog log("getWallClockDurationUntil", Log());
    LOGINFO << "when: " << when << std::endl;

    // If looping, add an offset to keep event times increasing.
    //
    Time::TimeStamp delta(when);

    // Obtain the playback clock time, and calculate the difference between the two. If the result is negative,
    // then the event has already happend according to the playback clock.
    //
    delta -= getPlaybackClock();
    LOGDEBUG << "delta: " << delta << std::endl;

    // We have the duration in playback time frame. Multiply by the playback rate to get the number of seconds
    // in the real time frame.
    //
    if (playbackClockRate_ != 1.0)
	delta *= playbackClockRate_;

    LOGDEBUG << "duration: " << delta << std::endl;
    return delta;
}

void
Clock::setClockRange(const Time::TimeStamp& start, const Time::TimeStamp& end)
{
    static Logger::ProcLog log("setClockRange", Log());
    LOGINFO << "start: " << start << " end: " << end << std::endl;
    minTime_ = start;
    maxTime_ = end;
    setPlaybackClockStart(start);
}

void
Clock::setWallClockRate(double wallClockRate)
{
    static Logger::ProcLog log("setWallClockRate", Log());
    LOGINFO << "wallClockRate: " << wallClockRate << std::endl;

    if (wallClockRate <= 0.0) {
	LOGERROR << "invalid wallClockRate: " << wallClockRate << std::endl;
	return;
    }

    // Since the rate is changing, we want to 'freeze' any past elapsed time at the old rate.
    //
    playbackClockStart_ = getPlaybackClock();

    // Reset the real time elapsed counter since our frame of reference has changed.
    //
    wallClockStart_ = Time::TimeStamp::Now();

    // Record the new rate.
    //
    wallClockRate_ = wallClockRate;
    playbackClockRate_ = 1.0 / wallClockRate;
}

void
Clock::setPlaybackClockStart(const Time::TimeStamp& playbackClockStart)
{
    static Logger::ProcLog log("setPlaybackClockStart", Log());
    LOGINFO << "playbackClockStart: " << playbackClockStart << std::endl;

    bool wasRunning = ticker_.isActive();
    if (wasRunning)
	stop();

    // Since we are setting a new starting time, discard any past elapsed time.
    //
    playbackClockStart_ = playbackClockStart;
    if (playbackClockStart_ < minTime_)
	playbackClockStart_ = minTime_;
    else if (playbackClockStart_ > maxTime_)
	playbackClockStart_ = maxTime_;

    emit playbackClockStartChanged(playbackClockStart);

    if (wasRunning)
	start();
}

void
Clock::start()
{
    static Logger::ProcLog log("start", Log());
    LOGINFO << std::endl;

    if (! ticker_.isActive()) {

	// Remember the current wall clock value, and then start the clock.
	//
	wallClockStart_ = Time::TimeStamp::Now();
	ticker_.start();
	emit started();
	doTick();
    }
}

void
Clock::stop()
{
    static Logger::ProcLog log("stop", Log());
    LOGINFO << std::endl;
    if (ticker_.isActive()) {
	playbackClockStart_ = getPlaybackClock();
	ticker_.stop();
	emit stopped();
    }
}

void
Clock::doTick()
{
    Time::TimeStamp playbackClock(getPlaybackClock());
    Time::TimeStamp elapsed(playbackClock);
    elapsed -= minTime_;
    emit tick(playbackClock, elapsed);
}

QString
Clock::formatTimeStamp(const Time::TimeStamp& when)
{
    return QString::fromStdString(when.hhmmss(2));
}

QString
Clock::formatDuration(const Time::TimeStamp& duration)
{
    return QString("+") + QString::fromStdString(duration.hhmmss(2));
}
