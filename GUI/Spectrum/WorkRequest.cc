#include "GUI/LogUtils.h"

#include "App.h"
#include "Configuration.h"
#include "FFTSettings.h"
#include "WeightWindow.h"
#include "WorkRequest.h"

using namespace SideCar::GUI::Spectrum;

Logger::Log&
WorkRequest::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("spectrum.WorkRequest");
    return log_;
}

WorkRequest::WorkRequest(const WeightWindow& weightWindow)
    : weightWindow_(weightWindow), gateStart_(0), inputSmoothing_(1),
      accumulated_(0), fftSize_(weightWindow.getSize()), input_(0),
      output_(0), plan_(0), master_(true), zeroPad_(false)
{
    FFTSettings* settings = App::GetApp()->getConfiguration()->getFFTSettings();

    gateStart_ = settings->getGateStart();
    inputSmoothing_ = settings->getInputSmoothing();
    zeroPad_ = settings->getZeroPad();

    connect(settings, SIGNAL(gateStartChanged(int)),
            SLOT(setGateStart(int)));

    connect(settings, SIGNAL(inputSmoothingChanged(int)),
            SLOT(setInputSmoothing(int)));

    connect(settings, SIGNAL(zeroPadChanged(bool)),
            SLOT(setZeroPad(bool)));

    initialize();
}

WorkRequest::WorkRequest(const WorkRequest& copy)
    : weightWindow_(copy.weightWindow_), gateStart_(copy.gateStart_),
      inputSmoothing_(copy.inputSmoothing_), accumulated_(0),
      fftSize_(copy.fftSize_), input_(0), output_(0), plan_(copy.plan_),
      master_(false), zeroPad_(copy.zeroPad_)
{
    FFTSettings* settings = App::GetApp()->getConfiguration()->getFFTSettings();

    connect(settings, SIGNAL(gateStartChanged(int)),
            SLOT(setGateStart(int)));

    connect(settings, SIGNAL(inputSmoothingChanged(int)),
            SLOT(setInputSmoothing(int)));

    connect(settings, SIGNAL(zeroPadChanged(bool)),
            SLOT(setZeroPad(bool)));

    initialize();
}

WorkRequest::~WorkRequest()
{
    if (input_) fftw_free(input_);
    if (output_) fftw_free(output_);
    if (plan_ && master_)
	fftw_destroy_plan(plan_);
}

void
WorkRequest::initialize()
{
    static Logger::ProcLog log("initialize", Log());

    if (input_)
	fftw_free(input_);
    input_ = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fftSize_);

    if (output_)
	fftw_free(output_);
    output_ = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fftSize_);

    if (master_) {
	if (plan_)
	    fftw_destroy_plan(plan_);
	plan_ = fftw_plan_dft_1d(fftSize_, input_, output_, FFTW_FORWARD,
                                 FFTW_MEASURE);
    }

    LOGDEBUG << "plan: " << plan_ << std::endl;
}

bool
WorkRequest::addData(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("addData", Log());
    LOGINFO << "accumulated: " << accumulated_ << std::endl;

    last_ = msg;

    size_t gateCount = msg->size() / 2;
    if (gateCount <= size_t(gateStart_))
	return false;

    gateCount -= gateStart_;
    size_t limit = fftSize_;
    if (limit > gateCount)
	limit = gateCount;

    size_t index = 0;

    const Messages::Video::DatumType* begin = &msg[0] + gateStart_ * 2;
    const Messages::Video::DatumType* end = &msg[0] + gateCount * 2;
    const Messages::Video::DatumType* ptr;

    if (inputSmoothing_ == 1) {
	while (index < fftSize_) {
	    for (ptr = begin; ptr < end && index < fftSize_; ++index) {
		float real = *ptr++ * weightWindow_[index];
		float imag = *ptr++ * weightWindow_[index];
		input_[index][0] = real;
		input_[index][1] = imag;
	    }

	    if (zeroPad_) {
		while (index < fftSize_) {
		    input_[index][0] = 0.0;
		    input_[index++][1] = 0.0;
		}
	    }
	}
    }
    else {
	++accumulated_;
	if (accumulated_ == 1) {
	    while (index < fftSize_) {
		for (ptr = begin; ptr < end && index < fftSize_; ++index) {
		    float real = *ptr++;
		    float imag = *ptr++;
		    input_[index][0] = real;
		    input_[index][1] = imag;
		}

		if (zeroPad_) {
		    while (index < fftSize_) {
			input_[index][0] = 0.0;
			input_[index++][1] = 0.0;
		    }
		}
	    }
	    return false;
	}
	else if (accumulated_ < inputSmoothing_) {
	    while (index < fftSize_) {
		for (ptr = begin; ptr < end && index < fftSize_; ++index) {
		    float real = *ptr++;
		    float imag = *ptr++;
		    input_[index][0] += real;
		    input_[index][1] += imag;
		}
		if (zeroPad_) {
		    while (index < fftSize_) {
			input_[index][0] = 0.0;
			input_[index++][1] = 0.0;
		    }
		}
	    }
	    return false;
	}
	else {
	    while (index < fftSize_) {
		for (ptr = begin; ptr < end && index < fftSize_; ++index) {
		    float weight = weightWindow_[index] / inputSmoothing_;
		    float real = input_[index][0] + *ptr++;
		    float imag = input_[index][1] + *ptr++;
		    input_[index][0] = real * weight;
		    input_[index][1] = imag * weight;
		}
		if (zeroPad_) {
		    while (index < fftSize_) {
			input_[index][0] = 0.0;
			input_[index++][1] = 0.0;
		    }
		}
	    }
	    accumulated_ = 0;
	}
    }

    return true;
}

void
WorkRequest::setGateStart(int value)
{
    accumulated_ = 0;
    gateStart_ = value;
}

void
WorkRequest::setInputSmoothing(int value)
{
    accumulated_ = 0;
    inputSmoothing_ = value;
}

void
WorkRequest::execute()
{
    static Logger::ProcLog log("execute", Log());
    if (! plan_) return;

    LOGDEBUG << this << " start FFT" << std::endl;

    if (master_)
	fftw_execute(plan_);
    else
	fftw_execute_dft(plan_, input_, output_);

    LOGDEBUG << this << " end FFT" << std::endl;
}
