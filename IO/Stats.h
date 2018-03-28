#ifndef SIDECAR_IO_STATS_H // -*- C++ -*-
#define SIDECAR_IO_STATS_H

#include "Time/TimeStamp.h"
#include <vector>

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** General processing statistics of an IO::Task. Records object and byte counts, and provides for calculating
    throughput rates in terms of messages and bytes.
*/
class Stats {
public:
    /** Obtain the log device to use for Stats objects.

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    Stats() { resetAll(); }

    /** Reset the counters and rates to zero.
     */
    void resetAll();

    void resetDropDupeCounts();

    /** Update the held counters.

        \param byteCount number of bytes processed
    */
    void updateInputCounters(size_t byteCount);

    /** Update the held counters.

        \param byteCount number of bytes processed

        \param sequenceNumber
    */
    void updateInputCounters(size_t byteCount, uint32_t sequenceNumber);

    /** Obtain the last-calculated byte rate.

        \return byte rate
    */
    size_t getByteRate() const { return byteRate_; }

    /** Obtain the last-calculated message rate.

        \return message rate
    */
    size_t getMessageRate() const { return messageRate_; }

    /** Obtain the running message count.

        \return message count
    */
    size_t getMessageCount() const { return messageTotal_; }

    /** Obtain the total number of dropped messages found so far.

        \return dropped count
    */
    size_t getDropCount() const { return drops_; }

    /** Obtain the total number of duplicate message IDs found so far.

        \return duplicate count
    */
    size_t getDupeCount() const { return dupes_; }

    /** Update the message and byte rates based on the elapsed time since the last update.
     */
    void calculateRates();

private:
    size_t byteCount_;
    size_t messageCount_;
    size_t byteRate_;
    size_t messageRate_;
    size_t messageTotal_;
    size_t drops_;
    size_t dupes_;
    uint32_t lastSequenceNumber_;
    size_t lastMessageTotal_;
    Time::TimeStamp lastRateCalcTime_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
