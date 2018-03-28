#ifndef SIDECAR_ALGORITHMS_DIFFERENCE_H
#define SIDECAR_ALGORITHMS_DIFFERENCE_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/**
   \ingroup Algorithms Converts from I/Q (cartesian) to polar coordinates.

   \par Pseudocode:
   - Buffer both channels
   - While both buffers are non-empty
   - Pop old entries that are only on one of the lists
   - Output the conversion of old entries that are on both lists

   \par Input Messages:
   - Messages::Video[0] I
   - Messages::Video[1] Q

   \par Output Messages:
   - Messages::Video[0] Amplitude
   - Messages::Video[1] Phase

   \par Run-time Parameters:
   none
*/
class IQ2AmplitudePhase : public Algorithm {
    // Algorithm interface
    //
public:
    IQ2AmplitudePhase(Controller& controller, Logger::Log& log);
    bool startup();
    bool process(Messages::Video::Ref, uint port);

private:
    // Video buffers
    //
    std::list<SideCar::Messages::Video::Ref> I;
    std::list<SideCar::Messages::Video::Ref> Q;
};

} // namespace Algorithms
} // namespace SideCar

/** \file
 */

#endif
