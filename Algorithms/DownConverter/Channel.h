#ifndef SIDECAR_ALGORITHMS_DOWNCONVERTER_CHANNEL_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_DOWNCONVERTER_CHANNEL_H

#include <deque>

#include "Algorithms/Processor.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include "DownConverter.h"

namespace SideCar {
namespace Algorithms {
namespace DownConverterUtils {

/** Utility class for DownConverter that manages data from one input channel. Contains an internal deque (FIFO)
    that allows the channel to synchronize with other Channel objects based on VME sequence counter value.
*/
class Channel : public TProcessor<Channel, Messages::Video> {
    using Super = TProcessor<Channel, Messages::Video>;

public:
    static Logger::Log& Log();

    /** Constructor for a DownConverter data channel.

        \param master

        \param maxBufferSize

        \param auxIndex
    */
    Channel(DownConverter& master, size_t maxBufferSize);

    size_t getBufferSize() const { return buffer_.size(); }

    /** Change the max buffer size.

        \param maxBufferSize new max buffer size
    */
    void setMaxBufferSize(size_t maxBufferSize);

    /** Reset to a know state.
     */
    void reset() { buffer_.clear(); }

    /** Determine if there are any messages in the internal buffer.

        \return true if so
    */
    bool isEmpty() const { return buffer_.empty(); }

    /** Obtain the sequence counter of the oldest message in the buffer. NOTE: not safe to call when buffer is
        empty.

        \return sequence counter of oldest message in the buffer.
    */
    uint32_t getNextSequence() const { return nextSequence_; }

    /** Add a new message to the channel's buffer.

        \param msg message to add

        \return true if
    */
    bool addData(const Messages::Video::Ref& msg);

    /** Remove entries that have VME sequence counter 'older' than a given one. Since sequence counter values
        should be monotonically increasing, except when wrapping around to zero, 'older' means a value that is
        less than the given value, or if the given value is zero, greater than the value + maxBufferSize.

        \param sequenceCounter the VME sequence counter to use for testing

        \return true if the channel is empty
    */
    bool prune(uint32_t sequenceCounter);

    void getSlice(int offset, int span, VsipComplexVector& slice);

    Messages::Video::Ref getData() const { return buffer_.front(); }

    Messages::Video::Ref popData();

private:
    void updateNextSequence();

    DownConverter& master_;
    std::deque<Messages::Video::Ref> buffer_;
    size_t maxBufferSize_;
    uint32_t nextSequence_;
};

} // end namespace DownConverterUtils
} // end namespace Algorithms
} // end namespace SideCar

#endif
