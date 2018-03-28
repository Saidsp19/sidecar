#include <cmath>

#include "Logger/Log.h"
#include "Stats.h"
#include "Time/TimeStamp.h"

using namespace SideCar;
using namespace SideCar::IO;

Logger::Log&
Stats::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Stats");
    return log_;
}

void
Stats::resetAll()
{
    byteCount_ = 0;
    messageCount_ = 0;
    byteRate_ = 0;
    messageRate_ = 0;
    messageTotal_ = 0;
    lastMessageTotal_ = 0;
    lastRateCalcTime_ = Time::TimeStamp::Min();
    resetDropDupeCounts();
}

void
Stats::resetDropDupeCounts()
{
    drops_ = 0;
    dupes_ = 0;
    lastSequenceNumber_ = 0;
}

void
Stats::updateInputCounters(size_t byteCount, uint32_t sequenceNumber)
{
    static Logger::ProcLog log("update", Log());
    LOGTIN << "byteCount: " << byteCount << " seq#: " << sequenceNumber << std::endl;

    // Asssume that the change between the last and new sequence counters is one, until shown otherwise. The
    // following will catch duplicate and positive changes; the situation where the new one is less than the old
    // one is treated as a delta of one, since it could be a Playback loop or a real-time rollover of the
    // sequence counter that caused the negative delta. If it is due to a runner glitch, the it will be caught
    // in a subsequent message as a gap.
    //
    size_t delta = 1;
    if (lastSequenceNumber_ != 0 && sequenceNumber >= lastSequenceNumber_) {
        delta = sequenceNumber - lastSequenceNumber_;
    }

    if (delta == 0) {
        ++dupes_;
        LOGERROR << "detected duplicate message - seq#: " << sequenceNumber << std::endl;
    } else if (delta > 1) {
        --delta;
        drops_ += delta;
        LOGERROR << "detected " << delta << " message drop(s) - seq#: " << sequenceNumber << std::endl;
    }

    lastSequenceNumber_ = sequenceNumber;
    updateInputCounters(byteCount);
}

void
Stats::updateInputCounters(size_t byteCount)
{
    byteCount_ += byteCount;
    ++messageCount_;
    ++messageTotal_;
}

void
Stats::calculateRates()
{
    static Logger::ProcLog log("calculateRates", Log());
    Time::TimeStamp now(Time::TimeStamp::Now());

    if (lastRateCalcTime_ == Time::TimeStamp::Min()) {
        lastRateCalcTime_ = now;
        return;
    }

    if (messageTotal_ == lastMessageTotal_) return;

    lastMessageTotal_ = messageTotal_;
    Time::TimeStamp delta(now);
    delta -= lastRateCalcTime_;
    double duration = delta.asDouble();
    LOGINFO << duration << std::endl;
    if (duration < 0.1) return;

    lastRateCalcTime_ = now;
    byteRate_ = size_t(::rint((byteRate_ + byteCount_ / duration) / 2.0));
    byteCount_ = 0;
    messageRate_ = size_t(::rint((messageRate_ + messageCount_ / duration) / 2.0));
    messageCount_ = 0;

    LOGDEBUG << "byteRate: " << byteRate_ << " messageRate: " << messageRate_ << std::endl;
}
