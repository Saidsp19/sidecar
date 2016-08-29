#ifndef SIDECAR_ALGORITHMS_MATCHEDFILTER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MATCHEDFILTER_H

#include <complex>

#include "ace/Message_Queue_T.h"
#include "boost/scoped_ptr.hpp"

#include <vsip/matrix.hpp>
#include <vsip/signal.hpp>
#include <vsip/vector.hpp>

#include "Algorithms/ManyInAlgorithm.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"

#include "MatchedFilterTypes.h"

namespace SideCar {
namespace Algorithms {

/** Documentation for the algorithm MatchedFilter. Please describe what the algorithm does, in layman's terms
    and, if possible, mathematical terms.
*/
class MatchedFilter : public ManyInAlgorithm
{
    using Super = ManyInAlgorithm;
public:

    enum InfoSlot {
	kFFTSize = Super::kNumSlots,
	kWorkerCount,
	kFFTThreadCount,
	kDomain,
	kNoPulseDetected,
	kNumSlots
    };

    /** Domain options available via the Master GUI application.
     */
    enum Domain {
	kMinValue,
	kFrequencyDomain = kMinValue, 
	kTimeDomain,
	kMaxValue = kTimeDomain,
	kNumFilterTypes
    };

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

    bool reset();
    
    void setTxPulseStartBin(int value)
	{ txPulseStartBin_->setValue(value); }

    void setTxPulseSpan(int value)
	{ txPulseSpan_->setValue(value); }

    void setTxThreshold(int value)
	{ txThreshold_->setValue(value); }

    void setTxThresholdStartBin(int value)
	{ txThresholdStartBin_->setValue(value); }

    void setTxThresholdSpan(int value)
	{ txThresholdSpan_->setValue(value); }
    
    void setRxFilterStartBin(int value)
	{ rxFilterStartBin_->setValue(value); }

    void setRxFilterSpan(int value)
	{ rxFilterSpan_->setValue(value); }

    void setFFTSize(int size)
	{ fftSize_->setValue(size); }

private:

    ChannelBuffer* makeChannelBuffer(int index, const std::string& name,
                                     size_t maxBufferSize);

    /** Start required worker threads based on runtime parameter.
     */
    void startThreads();

    /** Stop worker threads. Does not return until they are all stopped and joined.
     */
    void stopThreads();

    /** Override of Algorithm::getNumInfoSlots() that returns the total number of slots returned by a
        WindowedSLC object.

        \return slot count
    */
    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Override of Algorithm::setInfoSlots(). Stores XML representation of cancellation statistics into the
        given XML-RPC container.

        \param status XML-RPC container to hold the stats
    */
    void setInfoSlots(IO::StatusBase& status);

    /** Implementation of ManyInAlgorithm::processChannels(). Process messages from the input channel buffers.

	\returns true if no error; false otherwise
    */
    bool processChannels();

    /** Notification handler called when the numWorkers parameter changed. Restarts the worker threads so that
	there are the configured number

        \param parameter reference to parameter that changed
    */
    void numWorkersChanged(const Parameter::PositiveIntValue& parameter);

    /** Notification handler called when the numFFFThreads parameter changed. Restarts the worker threads so
	that there are the configured number

        \param parameter reference to parameter that changed
    */
    void numFFTThreadsChanged(const Parameter::PositiveIntValue& parameter);

    /** Notification handler called when the FFT size changed.

        \param parameter reference to parameter that changed
    */
    void fftSizeChanged(const Parameter::PositiveIntValue& parameter);

    /** Notification handler called when the rxFilterSpan parameter changed.

        \param parameter reference to parameter that changed
    */
    void rxFilterSpanChanged(const Parameter::IntValue& parameter);

    void buildFFTs();

    /** Determine if we need to restart our worker threads due to a parameter change. We only do so from within
        the process() method in order to prevent any race conditions while doing so. NOTE: this is a one-shot
        value, that resets restartWorkerThreads_ to FALSE before returning.

        \return true if so
    */
    bool getRestartWorkerThreads();

    /** Set the internal restartWorkerThreads_ attribute to TRUE so that a future getRestartWorkerThreads() will
	return TRUE in a thread-safe manner.
    */
    void setRestartWorkerThreads();

    /** Obtain a new WorkRequest object. NOTE: blocks if there are none available.

        \return ACE_Message_Block holding he new WorkRequest
    */
    ACE_Message_Block* getWorkRequest();

    /** 
     */
    TChannelBuffer<Messages::Video>* rx_;

    TChannelBuffer<Messages::Video>* tx_;

    /** Run-time parameter for the number of worker threads.
     */
    Parameter::PositiveIntValue::Ref numWorkers_;

    /** Run-time parameter for the number of FFT threads.
     */
    Parameter::PositiveIntValue::Ref numFFTThreads_;

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

    /** Run-time parameter indicating transmit pulse threshold over which this algorithm will simply pass the rx
	signal through unchanged
    */
    Parameter::IntValue::Ref txThreshold_;

    /** Run-time parameter indicating the start sample in the transmit signal in which to look for a transmit
        pulse.
    */
    Parameter::IntValue::Ref txThresholdStartBin_;

    /** Run-time parameter indicating the number of samples in the transmit signal in which to look for a
	transmit pulse
    */
    Parameter::IntValue::Ref txThresholdSpan_;

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

    boost::scoped_ptr<MatchedFilterUtils::FwdFFT> fwdFFT_;
    boost::scoped_ptr<VsipComplexVector> txPulseVec_;

    /** Worker thread pool generator.
     */
    boost::scoped_ptr<MatchedFilterUtils::WorkerThreads> workerThreads_;

    using WorkRequestQueue = MatchedFilterUtils::WorkRequestQueue;

    /** Thread-safe FIFO queue for work requests available to the producer thread.
     */
    boost::scoped_ptr<WorkRequestQueue> idleWorkRequests_;

    /** Thread-safe FIFO queue for work requests awaiting a worker thread to process them.
     */
    boost::scoped_ptr<WorkRequestQueue> pendingWorkRequests_;

    /** Thread-safe FIFO queue for completed work requests.
     */
    boost::scoped_ptr<WorkRequestQueue> finishedWorkRequests_;

    bool noPulseDetected_;

    bool restartWorkerThreads_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
