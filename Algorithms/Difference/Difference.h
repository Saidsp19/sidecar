#ifndef SIDECAR_ALGORITHMS_DIFFERENCE_H
#define SIDECAR_ALGORITHMS_DIFFERENCE_H

#include "Algorithms/Algorithm.h"
#include "Algorithms/SynchronizedBuffer.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms Calculates the difference of two channels.

   \par Pseudocode:
   - Buffer both channels
   - While both buffers are non-empty
   - Pop old entries that are only on one of the lists
   - Output the difference of old entries that are on both lists

   \par Input Messages:
   - Messages::Video[0] X1
   - Messages::Video[1] X2

   \par Output Messages:
   - Messages::Video Y=X1-X2

   \par Run-time Parameters:
   none
*/
class Difference : public Algorithm
{
public:

    Difference(Controller& controller, Logger::Log &log);

    bool startup();

    bool reset();

    bool processIn0(Messages::Video::Ref);

    bool processIn1(Messages::Video::Ref);

    bool process(Messages::Video::Ref in0, Messages::Video::Ref in1);

    void setBufferSize(size_t bufferSize)
	{ bufferSize_->setValue(bufferSize); }

private:

    /** Notification the the buffer size has been changed by an external entity.

        \param value new value
    */
    void bufferSizeChanged(const Parameter::PositiveIntValue& value);

    Parameter::PositiveIntValue::Ref bufferSize_;
    SynchronizedBuffer<Messages::Video> in0_;
    SynchronizedBuffer<Messages::Video> in1_;
};

}} // namespaces

/** \file
 */

#endif
