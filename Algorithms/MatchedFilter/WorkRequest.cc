#include <fftw3.h>

#include "Logger/Log.h"

#include "WorkRequest.h"

#include <vsip/domain.hpp>
#include <vsip/math.hpp>
#include <vsip/solvers.hpp>

using namespace SideCar;
using namespace SideCar::Algorithms;
using namespace SideCar::Algorithms::MatchedFilterUtils;

Logger::Log&
WorkRequest::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Algorithms.MatchedFilter.WorkRequest");
    return log_;
}

WorkRequest*
WorkRequest::FromMessageBlock(ACE_Message_Block* data)
{
    return reinterpret_cast<WorkRequest*>(data->base());
}

ACE_Message_Block*
WorkRequest::Make(VsipComplexVector* txPulseVec, int numFFTThreads, int fftSize)
{
    Logger::ProcLog log("Make", Log());

    // We use an ACE_Message_Block to hold a WorkRequest object so that we may use ACE's thread-safe message
    // queues to manage pending and idle work requests. First, allocate an ACE_Message_Block with enough space
    // for one WorkReqest.
    //
    ACE_Message_Block* data = new ACE_Message_Block(sizeof(WorkRequest));
    memset(data->base(), 0, sizeof(WorkRequest));

    // Use C++ placement new operator to instantiate a WorkRequest object inside the ACE_Message_Block. NOTE: by
    // doing this, we must manually call the WorkRequest destructor when we are done with the WorkRequest object
    // or else we will leak memory. See the Destroy() class method.
    //
    WorkRequest* tmp = new (data->base()) WorkRequest(txPulseVec, numFFTThreads, fftSize);

    return data;
}

void
WorkRequest::Destroy(ACE_Message_Block* data)
{
    Logger::ProcLog log("Destroy", Log());
    LOGINFO << data << ' ' << data->base() << std::endl;

    // !!! Since we used placement new above to constuct a WorkRequest with memory from an ACE_Message_Block, we
    // !!! must manually call the WorkRequest destructor.
    //
    WorkRequest* workRequest = FromMessageBlock(data);
    workRequest->~WorkRequest();
    data->release();
}

WorkRequest::WorkRequest(VsipComplexVector* txPulseVec, int numFFTThreads, int fftSize) :
    input_(), output_(), offset_(0), rxVec_(), fwdFFT_(), invFFT_()
{
    Logger::ProcLog log("WorkRequest", Log());
    LOGDEBUG << this << std::endl;
    reconfigure(txPulseVec, numFFTThreads, fftSize);
}

WorkRequest::~WorkRequest()
{
    Logger::ProcLog log("~WorkRequest", Log());
    LOGDEBUG << this << std::endl;
}

void
WorkRequest::reconfigure(VsipComplexVector* txPulseVec, int numFFTThreads, int fftSize)
{
    txPulseVec_ = txPulseVec;
    fftSize_ = fftSize;

    rxVec_.reset(new VsipComplexVector(fftSize));

    fftw_plan_with_nthreads(numFFTThreads);

    fwdFFT_.reset(new FwdFFT(vsip::Domain<1>(fftSize), 1.0));
    invFFT_.reset(new InvFFT(vsip::Domain<1>(fftSize), 1.0 / fftSize));

    fftw_plan_with_nthreads(1);

    input_.reset();
    output_.reset();
    offset_ = 0;
}

void
WorkRequest::beginRequest(const Messages::Video::Ref& input, const Messages::Video::Ref& output, size_t offset)
{
    input_ = input;
    output_ = output;
    offset_ = offset;
}

void
WorkRequest::process()
{
    static Logger::ProcLog log("process", Log());

    LOGINFO << "offset: " << offset_ << std::endl;

    // Create complex values from our input message.
    //
    Messages::Video::const_iterator inputPos = input_->begin() + offset_;
    Messages::Video::const_iterator inputEnd = input_->end();
    size_t inputSize = inputEnd - inputPos;
    if (inputSize > fftSize_ * 2) inputEnd = inputPos + fftSize_ * 2;

    int index = 0;
    while (inputPos < inputEnd) {
        float i = *inputPos++;
        float q = *inputPos++;
        rxVec_->put(index++, ComplexType(i, q));
    }

    // If necessary, pad the end with zeros.
    //
    while (index < fftSize_) { rxVec_->put(index++, ComplexType(0.0, 0.0)); }

    // Filter the data in-place.
    //
    (*rxVec_) = (*invFFT_)((*txPulseVec_) * (*fwdFFT_)((*rxVec_)));

    // Replace the appropriate output message samples with the complex component values from the above filtering
    // operation.
    //
    Messages::Video::iterator outputPos = output_->begin() + offset_;
    Messages::Video::iterator outputEnd = output_->end();
    size_t outputSize = outputEnd - outputPos;
    if (outputSize > fftSize_ * 2) outputEnd = outputPos + fftSize_ * 2;

    index = 0;
    while (outputPos < outputEnd) {
        *outputPos++ = Messages::Video::DatumType(rxVec_->get(index).real());
        *outputPos++ = Messages::Video::DatumType(rxVec_->get(index).imag());
        ++index;
    }
}
