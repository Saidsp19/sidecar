#ifndef SIDECAR_ALGORITHMS_MANYINCPIALGORITHM_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MANYINCPIALGORITHM_H

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
class ManyInCPIAlgorithm : public ManyInAlgorithm
{
    using Super = ManyInAlgorithm;

public:

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages

        \param cpiSpan size of the CPI in PRIs
    */
    ManyInCPIAlgorithm(Controller& controller, Logger::Log& log,
                       bool enabled = true, size_t maxBuffersize = 100,
                       size_t cpiSpan = 10);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters. Processors are
        registered in parent class.

        \return true if successful, false otherwise
    */
    bool startup();

protected:
    virtual bool processMessageReceived();

    /** Prune the channels of any incomplete CPIs. Note, "incomplete" does not include those CPIs whose messages
        are still arriving.

        \param the expected size of CPIs

        \returns true if all channels have a complete CPI
    */
    bool pruneToCPIBoundary(size_t cpiSpan);

    /** Verify the buffer contains enough PRIs to form a CPI

        \param the expected size of a CPI

        \returns if there are at least CPI + 1 messages in the channel buffer
    */
    bool verifyChannelSize(size_t cpiSpan);

    virtual bool processCPI() = 0;

    /** Run-time parameter for a channel's internal buffer size
     */
    Parameter::PositiveIntValue::Ref cpiSpan_;

private:

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
