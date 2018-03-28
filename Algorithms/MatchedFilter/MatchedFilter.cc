#include "boost/bind.hpp"

#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "Algorithms/Utils.h"
#include "Logger/Log.h"

#include "MatchedFilter.h"
#include "MatchedFilter_defaults.h"
#include "WorkRequestQueue.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kDomainNames[] = {"Frequency", "Time"};

const char* const*
MatchedFilter::DomainEnumTraits::GetEnumNames()
{
    return kDomainNames;
}

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
MatchedFilter::MatchedFilter(Controller& controller, Logger::Log& log) :
    Super(controller, log, kDefaultEnabled, kDefaultMaxBufferSize),
    txPulseStartBin_(Parameter::IntValue::Make("txPulseStartBin",
                                               "Start bin of reference pulse<br>"
                                               "(-1 for last, etc.)",
                                               kDefaultTxPulseStartBin)),
    txPulseSpan_(Parameter::IntValue::Make("txPulseSpan",
                                           "Number of bins for reference pulse<br>"
                                           "(&lt; 1 = msgSize + value)",
                                           kDefaultTxPulseSpan)),
    rxFilterStartBin_(Parameter::IntValue::Make("rxFilterStartBin",
                                                "Start bin of returns to filter<br>"
                                                "(-1 for last, etc.)",
                                                kDefaultRxFilterStartBin)),
    rxFilterSpan_(Parameter::IntValue::Make("rxFilterSpan",
                                            "Number of return bins to filter<br>"
                                            "(&lt; 1 = msgSize + value)",
                                            kDefaultRxFilterSpan)),
    fftSize_(
        Parameter::PositiveIntValue::Make("fftSize", "Size of FFT for filtering in frequency domain", kDefaultFftSize)),
    scaleWithSumMag_(Parameter::BoolValue::Make("scaleWithSumMag",
                                                "Scale pulse using sum of magnitudes instead of "
                                                "just max",
                                                kDefaultScaleWithSumMag)),
    domain_(DomainParameter::Make("domain_", "Domain", Domain(kDefaultDomain))), fwdFFT_(), invFFT_(), txPulseVec_(),
    rxVec_(), rxOp_()
{
    fftSize_->connectChangedSignalTo(boost::bind(&MatchedFilter::fftSizeChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// MatchedFilter class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
MatchedFilter::startup()
{
    bool ok = true;

    buildFFTs();

    ok = ok && registerParameter(txPulseStartBin_) && registerParameter(txPulseSpan_) &&
         registerParameter(rxFilterStartBin_) && registerParameter(rxFilterSpan_) && registerParameter(domain_) &&
         registerParameter(fftSize_) && registerParameter(scaleWithSumMag_);

    return ok && Super::startup();
}

bool
MatchedFilter::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

void
MatchedFilter::fftSizeChanged(const Parameter::PositiveIntValue& parameter)
{
    Logger::ProcLog log("fftSizeChanged", getLog());
    LOGINFO << "fftSize: " << parameter.getValue() << std::endl;
    buildFFTs();
}

void
MatchedFilter::buildFFTs()
{
    Logger::ProcLog log("buildFFTs", getLog());
    int fftSize = fftSize_->getValue();
    LOGINFO << "rebuilding FFTW with size " << fftSize << std::endl;
    fwdFFT_.reset(new FwdFFT(vsip::Domain<1>(fftSize), 1.0));
    invFFT_.reset(new InvFFT(vsip::Domain<1>(fftSize), 1.0 / fftSize));
    txPulseVec_.reset(new VsipComplexVector(fftSize));
    rxOp_.reset(new VsipComplexVector(fftSize));
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

    if (!isEnabled()) return send(rxMsg);

    // Since samples in the main and auxillary channels contain I,Q sample pairs, divide the message sizes by 2
    // to get the number of complex samples.
    //
    int txSize = txMsg->size() / 2;
    int rxSize = rxMsg->size() / 2;

    // Fetch the parameters that define the slice of the transmit message containing the transmit pulse.
    //
    int txPulseStart = txPulseStartBin_->getValue();
    int txPulseSpan = txPulseSpan_->getValue();

    LOGERROR << "txPulseStart : " << txPulseStart << " txPulseSpan : " << txPulseSpan << " txSize: " << txSize
             << std::endl;

    // Make sure txPulseStart and txPulseSpan have reasonable values given the number of samples in the message.
    // This will change txPulseStart and txPulseSpan if they are <= 0 so that they are valid sample indices and
    // lengths.
    //
    if (!Utils::normalizeSampleRanges(txPulseStart, txPulseSpan, txSize)) {
        getController().setError("txPulseStart is too large.");
        return true;
    }

    LOGERROR << "txPulseStart : " << txPulseStart << " txPulseSpan : " << txPulseSpan << " txSize: " << txSize
             << std::endl;

    int rxFilterStart = rxFilterStartBin_->getValue();
    int rxFilterSpan = rxFilterSpan_->getValue();

    // Make sure rxFilterStart and rxFilterSpan have reasonable values given the number of samples in the
    // message.
    //
    LOGERROR << "rxFilterStart: " << rxFilterStart << " rxFilterSpan: " << rxFilterSpan << " rxSize: " << rxSize
             << std::endl;

    if (!Utils::normalizeSampleRanges(rxFilterStart, rxFilterSpan, rxSize)) {
        getController().setError("rxFilterStart is too large.");
        return true;
    }

    LOGERROR << "rxFilterStart: " << rxFilterStart << " rxFilterSpan: " << rxFilterSpan << " rxSize: " << rxSize
             << std::endl;

    // Create our output message now that we are done validating inputs.
    //
    Messages::Video::Ref out(Messages::Video::Make(getName(), rxMsg));
    Messages::Video::Container& outputData(out->getData());

    // Copy unfiltered data to output msg. Since we are handling real and complex values separately, we must
    // double the limiting factor.
    //
    for (int index = 0; index < rxFilterStart * 2; ++index) outputData.push_back(rxMsg[index]);

    if (domain_->getValue() == kFrequencyDomain) {
        LOGDEBUG << "processing in frequency domain" << std::endl;

        // Fetch the FFT parameters. The number of windows is the number of FFTs we must execute in order to
        // process the filter span. Using ::ceil to sure that numWindows covers the entire message.
        //
        const int fftSize = fftSize_->getValue();
        const int numWindows = static_cast<int>(::ceil(static_cast<float>(rxFilterSpan) / fftSize));
        const int totalFFTSize = fftSize * numWindows;
        const int padding = totalFFTSize - rxFilterSpan;

        // Get slice of the tx channel representing the transmit pulse.
        //
        Messages::Video::iterator pos(txMsg->begin());
        pos += txPulseStart * 2;
        for (int index = 0; index < txPulseSpan; ++index) {
            float i = *pos++;
            float q = *pos++;
            txPulseVec_->put(index, ComplexType(i, q));
        }

        // Pad the txPulseVec with zeroes so that it represents a length equal to fftSize
        //
        for (int index = txPulseSpan; index < fftSize; ++index) txPulseVec_->put(index, ComplexType(0.0, 0.0));

        // Get portion of receive channel to filter.
        //
        if (!rxVec_ || rxVec_->size() < static_cast<size_t>(totalFFTSize))
            rxVec_.reset(new VsipComplexVector(totalFFTSize));

        pos = rxMsg->begin();
        pos += rxFilterSpan * 2;
        for (int index = 0; index < rxFilterSpan; ++index) {
            float i = *pos++;
            float q = *pos++;
            rxVec_->put(index, ComplexType(i, q));
        }

        // Pad the rxVec with zeroes so that it represents a length equal to a numWindows * fftSize
        //
        static const ComplexType kZero(0.0, 0.0);
        for (int index = rxFilterSpan; index < totalFFTSize; ++index) rxVec_->put(index, kZero);

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

        // Convert TX pulse samples into frequency domain with forward FFT
        //
        *txPulseVec_ /= scale;
        *txPulseVec_ = (*fwdFFT_)(*txPulseVec_);
        *txPulseVec_ = vsip::conj(*txPulseVec_);

        // Process the RX buffer as windows of fftSize.
        //
        for (int windowIndex = 0; windowIndex < numWindows; ++windowIndex) {
            int offset = windowIndex * fftSize;

            // Copy the appropriate portion of the rxVec into the appropriate FFT buffer
            //
            for (int index = 0; index < fftSize; ++index) (*rxOp_)(index) = (*rxVec_)(offset + index);

            // Transform Rx data into frequency space, compute Rx * Tx, and translate result back into time
            // space.
            //
            (*rxOp_) = (*invFFT_)((*txPulseVec_) * (*fwdFFT_)(*rxOp_));

            // Copy filtered data into output message.
            //
            int limit = fftSize;
            if (windowIndex == numWindows - 1) limit -= padding;
            for (int index = 0; index < limit; ++index) {
                outputData.push_back(Messages::Video::DatumType((*rxOp_)(index).real()));
                outputData.push_back(Messages::Video::DatumType((*rxOp_)(index).imag()));
            }
        }

        // Copy any remaining unfiltered data to the output msg. Again with the doubling of the limiting factor.
        //
        for (size_t index = (rxFilterStart + rxFilterSpan) * 2; index < rxMsg->size(); ++index) {
            outputData.push_back(rxMsg[index]);
        }
    } // end if processing in frequency domain
    else {
        LOGDEBUG << "processing in time domain" << std::endl;

        // Get slice of the tx channel representing the transmit pulse
        //
        VsipComplexVector txPulseVec(txPulseSpan);
        // Get slice of the tx channel representing the transmit pulse.
        //
        Messages::Video::iterator pos(txMsg->begin());
        pos += txPulseStart * 2;
        for (int index = 0; index < txPulseSpan; ++index) {
            float i = *pos++;
            float q = *pos++;
            txPulseVec.put(index, ComplexType(i, q));
        }

        // Get portion of receive channel to filter
        //
        VsipComplexVector rxVec(rxFilterSpan);

        pos = rxMsg->begin();
        pos += rxFilterStart * 2;
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
        for (size_t index = (rxFilterStart + rxFilterSpan) * 2; index < rxMsg->size(); ++index) {
            outputData.push_back(rxMsg[index]);
        }
    } // end if processing in time domain

    bool rc = send(out);

    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
MatchedFilter::setInfoSlots(IO::StatusBase& status)
{
    Super::setInfoSlots(status);
    status[kFFTSize] = int(fftSize_->getValue());
    status[kDomain] = kDomainNames[domain_->getValue()];
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;

    return Algorithm::FormatInfoValue(QString("%1 FFT: %2 Domain: %3")
                                          .arg(ManyInAlgorithm::GetFormattedStats(status))
                                          .arg(status[MatchedFilter::kFFTSize])
                                          .arg(status[MatchedFilter::kDomain]));
}

// Factory function for the DLL that will create a new instance of the MatchedFilter class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
MatchedFilterMake(Controller& controller, Logger::Log& log)
{
    return new MatchedFilter(controller, log);
}
