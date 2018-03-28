#include "boost/bind.hpp"

#include "Algorithms/Controller.h"
#include "Logger/Log.h"

#include "CIntegrate.h"
#include "CIntegrate_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

static const char* kColorEncodingNames[] = {"HSV", "RGB"};

const char* const*
CIntegrate::ColorEncodingEnumTraits::GetEnumNames()
{
    return kColorEncodingNames;
}

CIntegrate::CIntegrate(Controller& controller, Logger::Log& log) :
    Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
    min_(Parameter::NonNegativeIntValue::Make("min", "Minimum magnitude value to normalize", kDefaultMin)),
    max_(Parameter::NonNegativeIntValue::Make("max", "Maximum magnitude value to normalize", kDefaultMax)),
    numPRIs_(Parameter::PositiveIntValue::Make("numPRIs", "# PRIs to integrate", kDefaultNumPRIs)),
    colorEncoding_(
        ColorEncodingParameter::Make("colorEncoding_", "Color Encoding", ColorEncoding(kDefaultColorEncoding))),
    moveSlidingWin_(Parameter::PositiveIntValue::Make("moveSlidingWin", "# PRIs to advance window each CPI",
                                                      kDefaultMoveSlidingWin)),
    dopplerBin_(Parameter::NonNegativeIntValue::Make("dopplerBin", "Doppler bin to use for output channel",
                                                     kDefaultDopplerBin)),
    PRIs_(), fftOut_(), N_(kDefaultNumPRIs), maxSpan_(0), buffer_(), H_(),
    resultsIndex_(controller.getOutputChannelIndex("results")), rgbIndex_(controller.getOutputChannelIndex("rgb")),
    maxAmpIndex_(controller.getOutputChannelIndex("maxAmp")),
    dopplerBinOutIndex_(controller.getOutputChannelIndex("dopplerBinOutput")),
    kInvalidChannel_(getController().getNumOutputChannels())
{
    numPRIs_->connectChangedSignalTo(boost::bind(&CIntegrate::numPRIsChanged, this, _1));
    min_->connectChangedSignalTo(boost::bind(&CIntegrate::minValueChanged, this, _1));
    max_->connectChangedSignalTo(boost::bind(&CIntegrate::maxValueChanged, this, _1));
}

bool
CIntegrate::startup()
{
    registerProcessor<CIntegrate, Messages::Video>(&CIntegrate::processInput);

    scalingChanged();

    return Super::startup() && registerParameter(enabled_) && registerParameter(min_) && registerParameter(max_) &&
           registerParameter(numPRIs_) && registerParameter(moveSlidingWin_) && registerParameter(dopplerBin_) &&
           registerParameter(colorEncoding_);
}

bool
CIntegrate::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    if (!enabled_->getValue()) { return true; }

    // Check the need to resize buffers. We just handle growing
    //
    size_t msgSize = msg->size() / 2;
    if (msgSize > maxSpan_) {
        maxSpan_ = msgSize;
        LOGDEBUG << "new maxSpan: " << maxSpan_ << std::endl;
        dimensionsChanged();
    }

    // Add this new message to the message queue
    //
    buffer_.push_back(msg);
    if (buffer_.size() != N_) {
        // Not enough messages to process, so bail out
        //
        return true;
    }

    // Populate the input matrix
    //
    for (size_t rowIndex = 0; rowIndex < N_; ++rowIndex) {
        Messages::Video::Ref pri = buffer_[rowIndex];
        size_t M = pri->size() / 2;
        Messages::Video::const_iterator pos = pri->begin();
        VsipComplexMatrix::row_type row(PRIs_->row(rowIndex));
        size_t colIndex = 0;
        for (; colIndex < M; ++colIndex) {
            float r = *pos++;
            float i = *pos++;
            row.put(colIndex, ComplexType(r, i));
        }

        // Pad with zeroes if necessary
        //
        static const ComplexType zero(0.0, 0.0);
        for (; colIndex < maxSpan_; ++colIndex) row.put(colIndex, zero);
    }

    // Apply FFT on *columns* of the PRIs matrix. Results go into fftOut.
    //
    (*fftm_)(*PRIs_, *fftOut_);

    // Return results for mid point of the buffer
    //
    Messages::Video::Ref mid(buffer_[N_ / 2]);

    // Do maxAmp processing in I,Q space
    //
    Messages::Video::Ref maxAmp(Messages::Video::Make("CIntegrate", mid));
    processMaxAmp(maxAmp->getData());

    Messages::Video::Ref oneDop(Messages::Video::Make("CIntegrate", mid));
    processOneDopplerBin(oneDop->getData());

    // Compute the magnitudes of the PRIs
    //
    *fftOut_ = vsip::mag(*fftOut_);

    bool rc = true;

    // Create a new message to hold the output of what we do. Note that although we pass in the input message,
    // the new message does not contain any data.
    //
    Messages::Video::Ref out(Messages::Video::Make("CIntegrate", mid));
    out->resize(maxSpan_);

    // Send out the FFT results, one doppler bin's worth of data for each PRI
    //
    Messages::Video::Container& outputData(out->getData());
    for (size_t rowIndex = 0; rowIndex < N_ && rc; ++rowIndex) {
        VsipComplexMatrix::row_type row(fftOut_->row(rowIndex));
        for (size_t colIndex = 0; colIndex < maxSpan_; ++colIndex) {
            outputData[colIndex] = ::rint(row.get(colIndex).real());
        }
        rc = rc && send(out, resultsIndex_);
    }

    Messages::Video::Ref rgb(Messages::Video::Make("CIntegrate", mid));
    if (colorEncoding_->getValue() == kRGB) {
        processRGB(rgb->getData());
    } else {
        processHSV(rgb->getData());
    }

    // After CPI processing, remove W PRIs from buffer (sliding a 'window'). Send the same RGB, MaxAmp, and
    // Doppler bin slice messages for each message removed.
    //
    size_t W = moveSlidingWin_->getValue();
    for (size_t i = 0; i < W && !buffer_.empty(); ++i) {
        // Grab oldest message. Created messages will inherit its azimuth and other settings.
        //
        Messages::Video::Ref oldest(buffer_.front());
        buffer_.pop_front();

        if (rc && rgbIndex_ != kInvalidChannel_) {
            out = Messages::Video::Make("CIntegrate", oldest);
            out->getData() = rgb->getData();
            rc = rc && send(out, rgbIndex_);
        }

        if (rc && maxAmpIndex_ != kInvalidChannel_) {
            out = Messages::Video::Make("CIntegrate", oldest);
            out->getData() = maxAmp->getData();
            rc = rc && send(out, maxAmpIndex_);
        }

        if (rc && dopplerBinOutIndex_ != kInvalidChannel_) {
            out = Messages::Video::Make("CIntegrate", oldest);
            out->getData() = oneDop->getData();
            rc = rc && send(out, dopplerBinOutIndex_);
        }
    }

    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
CIntegrate::processHSV(Messages::Video::Container& outputData)
{
    static Logger::ProcLog log("processHSV", getLog());
    LOGINFO << std::endl;

    struct RGBType {
        RGBType() : R(0.0), G(0.0), B(0.0) {}
        RGBType(float r, float g, float b) : R(r), G(g), B(b) {}
        void operator+=(const RGBType& rhs) { R += rhs.R, G += rhs.G, B += rhs.B; }
        float R, G, B;
    };

    struct HSVType {
        HSVType(float h, float s, float v) : H(h), S(s), V(v) {}
        const float H, S, V;
    };

    for (size_t colIndex = 0; colIndex < maxSpan_; ++colIndex) {
        VsipComplexMatrix::col_type col(fftOut_->col(colIndex));

        RGBType sum;
        for (size_t rowIndex = 0; rowIndex < N_; ++rowIndex) {
            //
            // scale amplitude data so values fall between 0 and 1
            //
            const ComplexType& z = col(rowIndex);
            double amp = scaleAmp(z.real());

            // Come up with an hsv value for this doppler-range cell
            //
            HSVType hsv(H_[rowIndex], 1.0, amp);

            // Convert to RGB
            //
            int index = ::floor(hsv.H);
            float f = hsv.H - index;
            if (!(index & 1)) f = 1 - f; // if index is even
            float m = hsv.V * (1 - hsv.S);
            float n = hsv.V * (1 - hsv.S * f);

            switch (index) {
            case 6:
            case 0: sum += RGBType(hsv.V, n, m); break;
            case 1: sum += RGBType(n, hsv.V, m); break;
            case 2: sum += RGBType(m, hsv.V, n); break;
            case 3: sum += RGBType(m, n, hsv.V); break;
            case 4: sum += RGBType(n, m, hsv.V); break;
            case 5: sum += RGBType(hsv.V, m, n); break;
            }
        }

        // Push and encoded RGB value into the output buffer
        //
        outputData.push_back(Messages::Video::DatumType(EncodeRGB(sum.R, sum.G, sum.B)));
    }
}

void
CIntegrate::processRGB(Messages::Video::Container& outputData)
{
    static Logger::ProcLog log("processRGB", getLog());

    // Compute the RGB values for each range across the CPI
    //
    double span = N_ / 3.0;    // 3 components to work with: R, G, B
    double shift = span / 2.0; // The G component uses the highest and lowest
                               // bits, so everything is rotated 1/2 a span

    // Compute the summations
    //
    for (size_t i = 0; i < maxSpan_; ++i) {
        VsipComplexMatrix::col_type col(fftOut_->col(i));

        // Build our red, green, and blue components using values from the FFT output. Green is centered around
        // the 0 doppler bin and thus wraps around the buffer.
        //
        size_t j = 0;
        float G = 0.0;
        for (size_t limit = ::rint(shift); j < limit; ++j) G += scaleAmp(ComplexType(col(j)).real());
        float B = 0.0;
        for (size_t limit = ::rint(shift + span); j < limit; ++j) B += scaleAmp(ComplexType(col(j)).real());
        float R = 0.0;
        for (size_t limit = ::rint(shift + span * 2); j < limit; ++j) R += scaleAmp(ComplexType(col(j)).real());
        for (; j < N_; ++j) G += scaleAmp(ComplexType(col(j)).real());

        // Push data into the output buffer
        //
        outputData.push_back(EncodeRGB(R, G, B));
    }
}

bool
CIntegrate::reset()
{
    buffer_.clear();
    return true;
}

void
CIntegrate::processOneDopplerBin(Messages::Video::Container& outputData)
{
    static Logger::ProcLog log("processOneDopplerBin", getLog());

    // Get the I, Q values for each range in the specified doppler bin.
    //
    int dopplerBin = dopplerBin_->getValue();
    VsipComplexMatrix::row_type row(fftOut_->row(dopplerBin));
    for (size_t i = 0; i < maxSpan_; ++i) {
        // !!! Is truncation really desired vs. rounding?
        //
        uint16_t real = uint16_t(row.get(i).real());
        uint16_t imag = uint16_t(row.get(i).imag());
        outputData.push_back(real);
        outputData.push_back(imag);
    }
}

void
CIntegrate::processMaxAmp(Messages::Video::Container& outputData)
{
    // Compute the max amplitude values for each range across the CPI
    //
    for (size_t colIndex = 0; colIndex < maxSpan_; ++colIndex) {
        // Fetch the column
        //
        VsipComplexMatrix::col_type col(fftOut_->col(colIndex));

        // Fetch the index of the entry with the largest magnitude^2 value
        //
        vsip::Index<1> index;
        double maxM2 = vsip::maxmgsqval(col, index);

        // Fetch the value at the found index and store in the message.
        //
        uint16_t real = uint16_t(col.get(index[0]).real());
        uint16_t imag = uint16_t(col.get(index[0]).imag());
        outputData.push_back(real);
        outputData.push_back(imag);
    }
}

void
CIntegrate::numPRIsChanged(const Parameter::PositiveIntValue& parameter)
{
    dimensionsChanged();
}

void
CIntegrate::dimensionsChanged()
{
    static Logger::ProcLog log("dimensionsChanged", getLog());
    LOGINFO << std::endl;

    N_ = numPRIs_->getValue();

    LOGDEBUG << "N: " << N_ << " maxSpan: " << maxSpan_ << std::endl;

    // Only allocate when we've seen the first PRI message (which defines maxSpan_)
    //
    if (maxSpan_) {
        PRIs_.reset(new VsipComplexMatrix(N_, maxSpan_));
        fftOut_.reset(new VsipComplexMatrix(N_, maxSpan_));

        // Create FFT processor with a scaling of 1.0 / N. Although the matrix defined is N_ rows by maxSpan_
        // range bins, the ForwardFFTM type dictates that this will perform maxSpan_ FFTs, each of N_ samples;
        // in other words, each FFT process samples at the same range but at increasing time. NOTE: for faster
        // processing, look into rolling the 1.0 / N scaling into another operation where we don't have to visit
        // every sample.
        //
        fftm_.reset(new ForwardFFTM(vsip::Domain<2>(N_, maxSpan_), 1.0 / N_));
    }

    // This is for HSV encoding. Map each doppler slice to a distinct hue, starting with RGB green (0.3333 Hue)
    // for doppler bin 0.
    //
    H_.clear();
    H_.reserve(N_);
    double step = 1.0 / N_;
    for (size_t i = 0; i < N_; ++i) {
        H_.push_back(6.0 * ::fmod(1.0 / 3.0 + step * i, 1.0));
        LOGDEBUG << "H_[" << i << "]: " << H_.back() << std::endl;
    }

    reset();
}

void
CIntegrate::minValueChanged(const Parameter::NonNegativeIntValue& parameter)
{
    scalingChanged();
}

void
CIntegrate::maxValueChanged(const Parameter::NonNegativeIntValue& parameter)
{
    scalingChanged();
}

void
CIntegrate::scalingChanged()
{
    scaleMin_ = min_->getValue();
    float scaleMax = max_->getValue();
    if (scaleMax <= scaleMin_) {
        scaleMax = scaleMin_ + 1.0;
        max_->setValue(scaleMax);
    }

    scaleFactor_ = 1.0 / (scaleMax - scaleMin_);
}

void
CIntegrate::setInfoSlots(IO::StatusBase& status)
{
    Algorithm::setInfoSlots(status);
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kMin, (int)min_->getValue());
    status.setSlot(kMax, (int)max_->getValue());
    status.setSlot(kNumPRIs, (int)N_);
    status.setSlot(kColorEncoding, int(colorEncoding_->getValue()));
    status.setSlot(kWindowShift, moveSlidingWin_->getValue());
    status.setSlot(kDopplerBin, dopplerBin_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[CIntegrate::kEnabled]) Algorithm::FormatInfoValue("Disabled");

    return Algorithm::FormatInfoValue(QString("Range: %1-%2  #PRI: %3  Shift: %4  Doppler: %5  Enc: %6")
                                          .arg(int(status[CIntegrate::kMin]))
                                          .arg(int(status[CIntegrate::kMax]))
                                          .arg(int(status[CIntegrate::kNumPRIs]))
                                          .arg(int(status[CIntegrate::kWindowShift]))
                                          .arg(int(status[CIntegrate::kDopplerBin]))
                                          .arg(kColorEncodingNames[int(status[CIntegrate::kColorEncoding])]));
}

// Factory function for the DLL that will create a new instance of the CIntegrate class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
CIntegrateMake(Controller& controller, Logger::Log& log)
{
    return new CIntegrate(controller, log);
}
