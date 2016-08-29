#ifndef SIDECAR_ALGORITHMS_RANGEDOPPLERMAP_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_RANGEDOPPLERMAP_H

#include <deque>
#include <vector>

#include "boost/scoped_ptr.hpp"

#include <vsip/support.hpp>
#include <vsip/matrix.hpp>
#include <vsip/vector.hpp>
#include <vsip/signal.hpp>
#include <vsip/math.hpp>

#include "Algorithms/CPIAlgorithm.h"
#include "Parameter/Parameter.h"
#include "Messages/Video.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm %s. Please describe what the algorithm does, in layman's terms and, if
    possible, mathematical terms.
*/


class RangeDopplerMap : public CPIAlgorithm
{
public:
  
    using Super = CPIAlgorithm;
    using ComplexType = std::complex<float>;
    using VsipComplexVector = vsip::Vector<ComplexType>;
    using VsipComplexMatrix = vsip::Matrix<ComplexType>;
    using ForwardFFTM = vsip::Fftm<ComplexType,ComplexType,vsip::col, vsip::fft_fwd, vsip::by_reference>;

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    RangeDopplerMap(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

private:

    bool cpiSpanChanged(const Parameter::PositiveIntValue& parameter);
    bool processCPI();
    bool resize(int);
  
    // Add attributes here
    //
    boost::scoped_ptr<VsipComplexMatrix> CPI_;
    boost::scoped_ptr<VsipComplexMatrix> fft_output_;
    boost::scoped_ptr<VsipComplexMatrix> HammingWindow_;

    std::vector<float> hamming_;

    size_t maxSpan_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
