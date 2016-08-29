#ifndef SIDECAR_ALGORITHMS_SUMMER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SUMMER_H

#include "Algorithms/ChannelBuffer.h"
#include "Algorithms/ManyInAlgorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm Summer. Generates summed output of N input messages from different sources.
    The messages used in the summation will all have the same sequence counter value. Also, if an enabled
    channel has somehow missed a message (a message 'drop'), then there will not be an summed output for that
    sequence number. In other words, the sequence number must be present on all enabled channels for the
    algorithm to emit a summed messages.
*/
class Summer : public ManyInAlgorithm
{
    using Super = ManyInAlgorithm;
    using VideoChannelBuffer = TChannelBuffer<Messages::Video>;

public:

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Summer(Controller& controller, Logger::Log& log);

private:

    /** Create a new ChannelBuffer object for Video messages. Also creates and registers a BoolParameter object
        for runtime editing of the enabled state of the ChannelBuffer object.

        \param channelIndex the index of the channel to create

        \param maxBufferSize the maximum number of messages to buffer

        \return new ChannelBuffer object
    */
    ChannelBuffer* makeChannelBuffer(int channelIndex, const std::string& name,
                                     size_t maxBufferSize);

    /** Implementation of ManyInAlgorithm::processChannels() method. Creates a mesage with summed sample values
        and emits it.

        \return 
    */
    bool processChannels();
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
