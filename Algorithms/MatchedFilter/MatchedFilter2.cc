#include "boost/bind.hpp"

#include <fftw3.h>

#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Algorithms/Utils.h"
#include "Logger/Log.h"

#include "MatchedFilter2.h"
#include "MatchedFilter_defaults.h"
#include "WorkRequestQueue.h"
#include "WorkerThreads.h"

#include "QtCore/QString"
#include "QtCore/QVariant"

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::MatchedFilterUtils;

static const char* kDomainNames[] = {"Frequency", "Time"};

const char* const*
MatchedFilter::DomainEnumTraits::GetEnumNames()
{
    return kDomainNames;
}

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
MatchedFilter::MatchedFilter(Controller& controller, Logger::Log& logger) :
    Super(controller, logger, kDefaultEnabled, kDefaultMaxBufferSize),
    numWorkers_(
        Parameter::PositiveIntValue::Make("numWorkers", "Number of worker threads for processing", kDefaultNumWorkers)),
    numFFTThreads_(Parameter::PositiveIntValue::Make("numFFTThreads", "Number of FFT threads for processing",
                                                     kDefaultNumFFTThreads)),
    txPulseStartBin_(Parameter::IntValue::Make("txPulseStartBin",
                                               "First complex bin of reference pulse in Tx<br>"
                                               "(-1 for last, etc.)",
                                               kDefaultTxPulseStartBin)),
    txPulseSpan_(Parameter::IntValue::Make("txPulseSpan",
                                           "Number of complex bins for reference pulse in Tx<br>"
                                           "(&lt; 1 = msgSize + value)",
                                           kDefaultTxPulseSpan)),
    rxFilterStartBin_(Parameter::IntValue::Make("rxFilterStartBin",
                                                "First complex bin to filter in Rx<br>"
                                                "(-1 for last, etc.)",
                                                kDefaultRxFilterStartBin)),
    rxFilterSpan_(Parameter::IntValue::Make("rxFilterSpan",
                                            "Number of complex bins to filter in Rx<br>"
                                            "(&lt; 1 = msgSize + value)",
                                            kDefaultRxFilterSpan)),
    fftSize_(Parameter::PositiveIntValue::Make("fftSize",
                                               "Size of FFT for filtering in frequency domain<br>"
                                               "(best if power of 2)",
                                               kDefaultFftSize)),
    scaleWithSumMag_(Parameter::BoolValue::Make("scaleWithSumMag",
                                                "Scale Tx pulse using sum of magnitudes instead of "
                                                "just max value",
                                                kDefaultScaleWithSumMag)),
    domain_(DomainParameter::Make("domain_", "Domain", Domain(kDefaultDomain))),
    txThreshold_(Parameter::IntValue::Make("txThreshold",
                                           "If no Tx pulse values pass threshold,<br>"
                                           "pass unfiltered Rx",
                                           kDefaultTxThreshold)),
    txThresholdStartBin_(Parameter::IntValue::Make("txThresholdStartBin", "First complex bin for Tx pulse search",
                                                   kDefaultTxThresholdStartBin)),
    txThresholdSpan_(Parameter::IntValue::Make("txThresholdSpan", "Number of complex bins to search for Tx pulse",
                                               kDefaultTxThresholdSpan)),
    fwdFFT_(), txPulseVec_(), workerThreads_(0), idleWorkRequests_(new WorkRequestQueue("idle")),
    pendingWorkRequests_(new WorkRequestQueue("pending")), finishedWorkRequests_(new WorkRequestQueue("finished")),
    noPulseDetected_(false), restartWorkerThreads_(true)
{
    Logger::ProcLog log("MatchedFilter", logger);

    if (!fftw_init_threads()) LOGERROR << "failed fftw_init_threads" << std::endl;

    numWorkers_->connectChangedSignalTo(boost::bind(&MatchedFilter::numWorkersChanged, this, _1));
    fftSize_->connectChangedSignalTo(boost::bind(&MatchedFilter::fftSizeChanged, this, _1));
    rxFilterSpan_->connectChangedSignalTo(boost::bind(&MatchedFilter::rxFilterSpanChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// MatchedFilter class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
MatchedFilter::startup()
{
    return registerParameter(txPulseStartBin_) && registerParameter(txPulseSpan_) &&
           registerParameter(rxFilterStartBin_) && registerParameter(rxFilterSpan_) && registerParameter(domain_) &&
           registerParameter(fftSize_) && registerParameter(scaleWithSumMag_) && registerParameter(numWorkers_) &&
           registerParameter(numFFTThreads_) && registerParameter(txThreshold_) &&
           registerParameter(txThresholdStartBin_) && registerParameter(txThresholdSpan_) && Super::startup();
}

bool
MatchedFilter::shutdown()
{
    // Note: at this point there is no algorithm thread running, so this is safe to do here.
    //
    stopThreads();

    // When stopThreads() returns, no worker threads will be running so we can safely delete all WorkRequest
    // objects.
    //
    while (!idleWorkRequests_->is_empty()) {
        ACE_Message_Block* data;
        idleWorkRequests_->dequeue_head(data);
        WorkRequest::Destroy(data);
    }

    return Super::shutdown();
}

bool
MatchedFilter::reset()
{
    Logger::ProcLog log("reset", getLog());
    LOGINFO << std::endl;

    // First, signal the process() method to restart our worker threads.
    //
    setRestartWorkerThreads();

    return Super::reset();
}

ACE_Message_Block*
MatchedFilter::getWorkRequest()
{
    // Fetch from the idleQueue_ if possible. Otherwise, create a new WorkRequest.
    //
    ACE_Message_Block* data = 0;
    if (idleWorkRequests_->message_count() && idleWorkRequests_->dequeue_head(data) != -1) return data;
    return WorkRequest::Make(txPulseVec_.get(), numFFTThreads_->getValue(), fftSize_->getValue());
}

bool
MatchedFilter::getRestartWorkerThreads()
{
    // Safely acquire the restart signal, and reset it.
    //
    pendingWorkRequests_->lock().acquire();
    bool value = restartWorkerThreads_;
    restartWorkerThreads_ = false;
    pendingWorkRequests_->lock().release();
    return value;
}

void
MatchedFilter::setRestartWorkerThreads()
{
    // Safely set the restart signal.
    //
    pendingWorkRequests_->lock().acquire();
    restartWorkerThreads_ = true;
    pendingWorkRequests_->lock().release();
}

void
MatchedFilter::startThreads()
{
    Logger::ProcLog log("startThreads", getLog());
    LOGINFO << std::endl;

    size_t numWorkers = numWorkers_->getValue();

    // Visit existing WorkRequest objects, updating them with new parameter values.
    //
    for (size_t index = 0; index < idleWorkRequests_->message_count(); ++index) {
        // Remove top item from queue, update with new values, and push to back of queue. This should visit all
        // of the item in the idle queue.
        //
        ACE_Message_Block* data;
        idleWorkRequests_->dequeue_head(data);
        WorkRequest::FromMessageBlock(data)->reconfigure(txPulseVec_.get(), numFFTThreads_->getValue(),
                                                         fftSize_->getValue());
        idleWorkRequests_->enqueue_tail(data);
    }

    // Create new thread pool object.
    //
    workerThreads_.reset(new WorkerThreads(*this, *pendingWorkRequests_, *finishedWorkRequests_));
    LOGDEBUG << "attempting to start " << numWorkers << " worker threads" << std::endl;

    int rc = workerThreads_->activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED, numWorkers);

    if (rc == -1) { LOGERROR << "failed to activate worker threads" << std::endl; }
}

void
MatchedFilter::stopThreads()
{
    Logger::ProcLog log("stopThreads", getLog());
    LOGINFO << std::endl;

    // Signal all threads to stop. Processing threads will also stop once they become idle.
    //
    pendingWorkRequests_->deactivate();
    if (workerThreads_) {
        // Wait until all threads have stopped.
        //
        workerThreads_->wait();
        LOGDEBUG << "threads stopped" << std::endl;
        workerThreads_.reset();
    }

    // Now that there are no worker threads around, reactivate the queue and move any pending requests back onto
    // the idle queue.
    //
    pendingWorkRequests_->activate();
    while (!pendingWorkRequests_->is_empty()) {
        ACE_Message_Block* data;
        pendingWorkRequests_->dequeue_head(data);
        idleWorkRequests_->enqueue_tail(data);
    }

    // Move any finished requests back onto the idle queue.
    //
    while (!finishedWorkRequests_->is_empty()) {
        ACE_Message_Block* data;
        finishedWorkRequests_->dequeue_head(data);
        idleWorkRequests_->enqueue_tail(data);
    }
}

void
MatchedFilter::numWorkersChanged(const Parameter::PositiveIntValue& parameter)
{
    setRestartWorkerThreads();
}

void
MatchedFilter::fftSizeChanged(const Parameter::PositiveIntValue& parameter)
{
    Logger::ProcLog log("fftSizeChanged", getLog());
    LOGINFO << "fftSize: " << parameter.getValue() << std::endl;
    setRestartWorkerThreads();
}

void
MatchedFilter::rxFilterSpanChanged(const Parameter::IntValue& parameter)
{
    Logger::ProcLog log("rxFilterSpanChanged", getLog());
    LOGINFO << "rxFilterSpan: " << parameter.getValue() << std::endl;
    setRestartWorkerThreads();
}

void
MatchedFilter::buildFFTs()
{
    Logger::ProcLog log("buildFFTs", getLog());
    int fftSize = fftSize_->getValue();
    LOGINFO << "rebuilding FFTW with size " << fftSize << std::endl;

    fftw_plan_with_nthreads(numFFTThreads_->getValue());
    fwdFFT_.reset(new FwdFFT(vsip::Domain<1>(fftSize), 1.0));
    fftw_plan_with_nthreads(1);

    txPulseVec_.reset(new VsipComplexVector(fftSize));
}

ChannelBuffer*
MatchedFilter::makeChannelBuffer(int index, const std::string& name, size_t maxBufferSize)
{
    static Logger::ProcLog log("makeChannelBuffer", getLog());
    LOGINFO << std::endl;

    if (name == "transmit") {
        tx_ = new TChannelBuffer<Messages::Video>(*this, index, maxBufferSize);
        return tx_;
    } else if (name == "receive") {
        rx_ = new TChannelBuffer<Messages::Video>(*this, index, maxBufferSize);
        return rx_;
    } else {
        LOGFATAL << "invalid channel name - " << name << std::endl;
    }

    return 0;
}

bool
MatchedFilter::processChannels()
{
    static Logger::ProcLog log("processChannels", getLog());
    LOGINFO << std::endl;

    Messages::Video::Ref rxMsg(rx_->popFront());
    Messages::Video::Ref txMsg(tx_->popFront());

    if (!isEnabled()) {
        LOGDEBUG << "not enabled" << std::endl;
        return send(rxMsg);
    }

    // Since samples in the main and auxillary channels contain I,Q sample pairs, divide the message sizes by 2
    // to get the number of complex samples.
    //
    int txSize = txMsg->size() / 2;
    int rxSize = rxMsg->size() / 2;

    // Check Tx for a pulse above a given threshold.
    //
    int txThreshold = txThreshold_->getValue();
    int txThresholdStart = txThresholdStartBin_->getValue();
    int txThresholdSpan = txThresholdSpan_->getValue();
    LOGDEBUG << "txThreshold: " << txThreshold << " txThresholdStart: " << txThresholdStart
             << " txThresholdSpan: " << txThresholdSpan << " txSize: " << txSize << std::endl;
    if (!Utils::normalizeSampleRanges(txThresholdStart, txThresholdSpan, txSize)) {
        getController().setError("txThresholdStart is too large.");
        return true;
    }

    bool txPulseDetected = false;
    Messages::Video::const_iterator pos(txMsg->begin());
    pos += txThresholdStartBin_->getValue() * 2;
    for (int index = 0; index < txThresholdSpan_->getValue(); ++index) {
        float i = *pos++;
        float q = *pos++;
        if (abs(i) > txThreshold || abs(q) > txThreshold) {
            txPulseDetected = true;
            break;
        }
    }

    if (!txPulseDetected) {
        LOGWARNING << "no pulse detected" << std::endl;
        noPulseDetected_ = true;
        return send(rxMsg);
    }

    noPulseDetected_ = false;

    // Check if we need to (re)start our worker threads. Done here to remove any chance of a race condition with
    // our message processing thread that runs this method.
    //
    if (getRestartWorkerThreads()) {
        if (workerThreads_) stopThreads();
        buildFFTs();
        startThreads();
    }

    // Fetch the parameters that define the slice of the transmit message containing the transmit pulse.
    //
    int txPulseStart = txPulseStartBin_->getValue();
    int txPulseSpan = txPulseSpan_->getValue();
    LOGDEBUG << "txPulseStart : " << txPulseStart << " txPulseSpan : " << txPulseSpan << " txSize: " << txSize
             << std::endl;

    // Make sure txPulseStart and txPulseSpan have reasonable values given the number of samples in the message.
    // This will change txPulseStart and txPulseSpan if they are <= 0 so that they are valid sample indices and
    // lengths.
    //
    if (!Utils::normalizeSampleRanges(txPulseStart, txPulseSpan, txSize)) {
        getController().setError("txPulseStart is too large.");
        return true;
    }

    LOGDEBUG << "txPulseStart : " << txPulseStart << " txPulseSpan : " << txPulseSpan << " txSize: " << txSize
             << std::endl;

    int rxFilterStart = rxFilterStartBin_->getValue();
    int rxFilterSpan = rxFilterSpan_->getValue();
    LOGDEBUG << "rxFilterStart : " << rxFilterStart << " rxFilterSpan : " << rxFilterSpan << " rxSize: " << rxSize
             << std::endl;

    // Make sure rxFilterStart and rxFilterSpan have reasonable values given the number of samples in the
    // message.
    //
    if (!Utils::normalizeSampleRanges(rxFilterStart, rxFilterSpan, rxSize)) {
        getController().setError("rxFilterStart is too large.");
        return true;
    }

    LOGDEBUG << "rxFilterStart : " << rxFilterStart << " rxFilterSpan : " << rxFilterSpan << " rxSize: " << rxSize
             << std::endl;

    // Create our output message now that we are done validating inputs.
    //
    Messages::Video::Ref out(Messages::Video::Make(getName(), rxMsg));
    Messages::Video::Container& outputData(out->getData());

    // Copy over samples before rxFilterStart.
    //
    std::copy(rxMsg->begin(), rxMsg->begin() + rxFilterStart * 2, std::back_inserter<>(outputData));

    if (domain_->getValue() == kFrequencyDomain) {
        LOGDEBUG << "processing in frequency domain" << std::endl;

        // Since different threads will muck with their own part of the output buffer, we need it to be its full
        // size.
        //
        outputData.resize(rxMsg->size());

        // Copy over samples after rxFilterStart + rxFilterSpan
        //
        size_t offset = (rxFilterStart + rxFilterSpan) * 2;
        std::copy(rxMsg->begin() + offset, rxMsg->end(), outputData.begin() + offset);

        // Fetch the FFT parameters. The number of windows is the number of FFTs we must execute in order to
        // process the filter span. Using ::ceil to sure that numWindows covers the entire message.
        //
        const int fftSize = fftSize_->getValue();
        int numWindows = static_cast<int>(::ceil(static_cast<float>(rxFilterSpan) / fftSize));
        const int totalFFTSize = fftSize * numWindows;
        const int padding = totalFFTSize - rxFilterSpan;

        // Get slice of the tx channel representing the transmit pulse.
        //
        Messages::Video::const_iterator pos(txMsg->begin());
        pos += 2 * txPulseStart;
        for (int index = 0; index < txPulseSpan; ++index) {
            float i = *pos++;
            float q = *pos++;
            txPulseVec_->put(index, ComplexType(i, q));
        }

        // Pad the txPulseVec with zeroes so that it represents a length equal to fftSize
        //
        for (int index = txPulseSpan; index < fftSize; ++index) txPulseVec_->put(index, ComplexType(0.0, 0.0));

        // Fetch the average magnitude found within the pulse. Since this value is always non-negative (sum of
        // squares), we don't need to use a complex type to represent it.
        //
        float scale;
        if (scaleWithSumMag_->getValue()) {
            scale = vsip::sqrt(vsip::sumval(vsip::magsq(*txPulseVec_)));
        } else {
            vsip::Index<1> maxIndex;
            scale = vsip::sqrt(vsip::maxmgsqval(*txPulseVec_, maxIndex));
        }

        // Convert xmit pulse into frequence domain.
        //
        *txPulseVec_ /= scale;
        *txPulseVec_ = (*fwdFFT_)(*txPulseVec_);
        *txPulseVec_ = vsip::conj(*txPulseVec_);

        // Process the rx buffer as windows of fftSize, each window processed by a separate thread.
        //
        offset = 2 * rxFilterStart;
        for (int windowIndex = 0; windowIndex < numWindows; ++windowIndex) {
            // Fetch a new/existing WorkRequest object and fill it with the window values.
            //
            ACE_Message_Block* data = getWorkRequest();
            WorkRequest* wr = WorkRequest::FromMessageBlock(data);
            wr->beginRequest(rxMsg, out, offset);
            offset += fftSize * 2;

            // Add to the pending queue for the next available WorkerThread to process.
            //
            pendingWorkRequests_->enqueue_tail(data);
        }

        // Now that we've scheduled all of the work, we need to essentially perform a barrier wait until all of
        // the threads have finished performing their calculations.
        //
        ACE_Message_Block* data = 0;
        while (finishedWorkRequests_->dequeue_head(data) != -1) {
            idleWorkRequests_->enqueue_tail(data);
            --numWindows;
            if (numWindows == 0) break;
        }
    } else {
        LOGDEBUG << "processing in time domain" << std::endl;

        // Get slice of the tx channel representing the transmit pulse
        //
        VsipComplexVector txPulseVec(txPulseSpan);

        // Get slice of the tx channel representing the transmit pulse.
        //
        Messages::Video::const_iterator pos(txMsg->begin());
        pos += 2 * txPulseStart;
        for (int index = 0; index < txPulseSpan; ++index) {
            float i = *pos++;
            float q = *pos++;
            txPulseVec.put(index, ComplexType(i, q));
        }

        // Get portion of receive channel to filter
        //
        VsipComplexVector rxVec(rxFilterSpan);

        pos = rxMsg->begin() + 2 * rxFilterStart;
        for (int index = 0; index < rxFilterSpan; ++index) {
            float i = *pos++;
            float q = *pos++;
            rxVec.put(index, ComplexType(i, q));
        }

        txPulseVec = conj(txPulseVec);

        float scale;
        if (scaleWithSumMag_->getValue()) {
            scale = vsip::sqrt(vsip::sumval(vsip::magsq(txPulseVec)));
        } else {
            vsip::Index<1> maxIndex;
            scale = vsip::sqrt(vsip::maxmgsqval(txPulseVec, maxIndex));
        }

        // Normalize the values of the transmit pulse
        //
        txPulseVec /= scale;

        int limit = rxFilterSpan - txPulseSpan;
        for (int index = 0; index < limit; ++index) {
            vsip::Domain<1> range(index, 1, txPulseSpan);
            VsipComplexVector rxOp = rxVec.get(range);
            rxVec(index) = vsip::sumval(txPulseVec * rxOp);
        }

        // Copy over the filtered data to output msg
        //
        for (int index = 0; index < rxFilterSpan - txPulseSpan; ++index) {
            outputData.push_back(Messages::Video::DatumType(::rint(rxVec.get(index).real())));
            outputData.push_back(Messages::Video::DatumType(::rint(rxVec.get(index).imag())));
        }

        // Copy any remaining unfiltered data to the output msg
        //
        for (size_t index = (rxFilterStart + rxFilterSpan - txPulseSpan) * 2; index < rxMsg->size(); ++index) {
            outputData.push_back(rxMsg[index]);
        }
    } // end if processing in time domain

    if (outputData.size() > 30 * 1024) LOGERROR << "abnormally large size: " << outputData.size() << std::endl;

    bool rc = send(out);

    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
MatchedFilter::setInfoSlots(IO::StatusBase& status)
{
    Super::setInfoSlots(status);
    status.setSlot(kFFTSize, fftSize_->getValue());
    status.setSlot(kWorkerCount, numWorkers_->getValue());
    status.setSlot(kFFTThreadCount, numFFTThreads_->getValue());
    status.setSlot(kDomain, domain_->getValue());
    status.setSlot(kNoPulseDetected, noPulseDetected_);
}

extern "C" ACE_Svc_Export QVariant
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return QVariant();

    if (!status[ManyInAlgorithm::kEnabled] || status[MatchedFilter::kNoPulseDetected]) return "Rx Passthrough";

    return QString("Threads: %1/%2 FFT: %3 Domain: %4 ")
               .arg(int(status[MatchedFilter::kWorkerCount]))
               .arg(int(status[MatchedFilter::kFFTThreadCount]))
               .arg(int(status[MatchedFilter::kFFTSize]))
               .arg(kDomainNames[int(status[MatchedFilter::kDomain])]) +
           ManyInAlgorithm::GetFormattedStats(status);
}

// Factory function for the DLL that will create a new instance of the MatchedFilter class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
MatchedFilterMake(Controller& controller, Logger::Log& log)
{
    return new MatchedFilter(controller, log);
}
