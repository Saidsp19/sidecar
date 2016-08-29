#ifndef SIDECAR_ALGORITHMS_LOWPASSFILTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_LOWPASSFILTER_H

#include "Algorithms/Algorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include <vector>
#include <complex>

#include <vsip/matrix.hpp>
#include <vsip/signal.hpp>
#include <vsip/vector.hpp>

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm LowPassFilter. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class LowPassFilter : public Algorithm
{
    using Super = Algorithm;
    using ComplexType = std::complex<float>;
    using VsipFloatVector = vsip::Vector<float>;
    using VsipComplexVector = vsip::Vector<ComplexType>;
    using VsipFIR = vsip::Fir<ComplexType>; 
    using FwdFFT = vsip::Fft<vsip::Vector,ComplexType,ComplexType,vsip::fft_fwd>;
    using InvFFT = vsip::Fft<vsip::Vector,ComplexType,ComplexType,vsip::fft_inv>;

public:

    enum InfoSlots {
        kEnabled = ControllerStatus::kNumSlots,
        kNumSlots
    };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    LowPassFilter(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

private:

    void fftSizeChanged(const Parameter::PositiveIntValue& parameter);
    size_t getNumInfoSlots() const { return kNumSlots; }
    void buildFFTs();
    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Video::Ref& msg);

    bool loadKernel();
    // Add attributes here
    //
    Parameter::BoolValue::Ref enabled_;
    Parameter::PositiveIntValue::Ref  fftSize_;
    Parameter::StringValue::Ref  kernelFile_;

    std::vector<Parameter::DoubleValue::Ref> alphas_;   

    boost::scoped_ptr<VsipComplexVector> msg_;   
    boost::scoped_ptr<VsipComplexVector> filtered_data_;   
    boost::scoped_ptr<VsipComplexVector> kernel_;   

    boost::scoped_ptr<FwdFFT> fwdFFT_;
    boost::scoped_ptr<InvFFT> invFFT_;
    boost::scoped_ptr<VsipComplexVector> txPulseVec_;
}; 

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
