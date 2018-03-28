#ifndef SIDECAR_ALGORITHMS_MATCHEDFILTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MATCHEDFILTER_H

#include <complex>

#include <vsip/matrix.hpp>
#include <vsip/signal.hpp>
#include <vsip/vector.hpp>

#include <fftw3.h>

#include "Algorithms/ManyInAlgorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm MatchedFilter. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class MatchedFilter : public ManyInAlgorithm {
    using Super = ManyInAlgorithm;
    using ComplexType = std::complex<float>;
    using VsipComplexVector = vsip::Vector<ComplexType>;
    using VsipComplexMatrix = vsip::Matrix<ComplexType>;
    using FwdFFT = vsip::Fft<vsip::Vector, ComplexType, ComplexType, vsip::fft_fwd>;
    using InvFFT = vsip::Fft<vsip::Vector, ComplexType, ComplexType, vsip::fft_inv>;

public:
    enum InfoSlot { kFFTSize = Super::kNumSlots, kOomain, kNumSlots };

    /** Domain options available via the Master GUI application.
     */
    enum Domain { kMinValue, kFrequencyDomain = kMinValue, kTimeDomain, kMaxValue = kTimeDomain, kNumFilterTypes };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    MatchedFilter(Controller& controller, Logger::Log& log);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    bool shutdown();

    void setTxPulseStartBin(int value) { txPulseStartBin_->setValue(value); }

    void setTxPulseSpan(int value) { txPulseSpan_->setValue(value); }

    void setRxFilterStartBin(int value) { rxFilterStartBin_->setValue(value); }

    void setRxFilterSpan(int value) { rxFilterSpan_->setValue(value); }

    void setFFTSize(int size) { fftSize_->setValue(size); }

private:
    ChannelBuffer* makeChannelBuffer(int index, const std::string& name, size_t maxBufferSize);

    void fftSizeChanged(const Parameter::PositiveIntValue& parameter);
    void buildFFTs();

    size_t getNumInfoSlots() const { return kNumSlots; }

    void setInfoSlots(IO::StatusBase& status);

    /** Process messages from the input channel buffers

        \returns true if no error; false otherwise
    */
    bool processChannels();

    TChannelBuffer<Messages::Video>* rx_;
    TChannelBuffer<Messages::Video>* tx_;

    /** Run-time parameter for the first complex sample in the TxRF message to use for the transmit pulse.
        Negative values cause the processing to start from the end of the message: -1 is the last gate, -2 the
        second-to-last, etc.
    */
    Parameter::IntValue::Ref txPulseStartBin_;

    /** Run-time parameter for the number of complex samples in the TxRF message to use for the transmit pulse,
        starting at txPulseStartBin_.
    */
    Parameter::IntValue::Ref txPulseSpan_;

    /** Run-time parameter for the first complex sample in the RxRF message to filter. Negative values cause the
        processing to start from the end of the message: -1 is the last gate, -2 the second-to-last, etc.
    */
    Parameter::IntValue::Ref rxFilterStartBin_;

    /** Run-time parameter for the number of complex samples in the RxRF message to filter, starting at
        rxFilterStartBin_.
    */
    Parameter::IntValue::Ref rxFilterSpan_;

    /** Run-time parameter that specifies the size of the FFTs to be used for filtering the data.
     */
    Parameter::PositiveIntValue::Ref fftSize_;

    /** Run-time parameter that controls which scaling factor to apply to the transmit pulse buffer.
     */
    Parameter::BoolValue::Ref scaleWithSumMag_;

    /** Definition of the enum range for the domain_ parameter.
     */
    struct DomainEnumTraits : public Parameter::Defs::EnumTypeTraitsBase {
        using ValueType = Domain;
        static ValueType GetMinValue() { return kMinValue; }
        static ValueType GetMaxValue() { return kMaxValue; }
        static const char* const* GetEnumNames();
    };

    using DomainParameter = Parameter::TValue<Parameter::Defs::Enum<DomainEnumTraits>>;

    /** Run-time parameter that determines the domain to perform the matched filter.
     */
    DomainParameter::Ref domain_;

    boost::scoped_ptr<FwdFFT> fwdFFT_;
    boost::scoped_ptr<InvFFT> invFFT_;
    boost::scoped_ptr<VsipComplexVector> txPulseVec_;
    boost::scoped_ptr<VsipComplexVector> rxVec_;
    boost::scoped_ptr<VsipComplexVector> rxOp_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
