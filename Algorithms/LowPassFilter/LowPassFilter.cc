#include "boost/bind.hpp"

#include <algorithm> // for std::transform
#include <fstream>
#include <functional> // for std::bind* and std::mem_fun*
#include <iostream>
#include <sstream>
#include <string>

#include "Logger/Log.h"

#include "LowPassFilter.h"
#include "LowPassFilter_defaults.h"

#include "QtCore/QString"

using namespace SideCar;
using namespace SideCar::Algorithms;

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur
// in the startup() method. NOTE: it is WRONG to call any virtual functions here...
//
LowPassFilter::LowPassFilter(Controller& controller, Logger::Log& log) :
    Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
    fftSize_(Parameter::PositiveIntValue::Make("fftSize", "Size of FFT", kDefaultFftSize)),
    kernelFile_(Parameter::StringValue::Make("kernelFile", "Path and name to kernel file", kDefaultKernelFile)),
    alphas_(), msg_(), filtered_data_(), kernel_()
{
    fftSize_->connectChangedSignalTo(boost::bind(&LowPassFilter::fftSizeChanged, this, _1));
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the
// LowPassFilter class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke
// Algorithm::startup() as shown below.
//
bool
LowPassFilter::startup()
{
    registerProcessor<LowPassFilter, Messages::Video>(&LowPassFilter::processInput);
    bool ok = true;

    kernel_.reset(new VsipComplexVector(8, ComplexType(0.0, 0.0)));
    filtered_data_.reset(new VsipComplexVector(128, ComplexType(0.0, 0.0)));
    msg_.reset(new VsipComplexVector(128, ComplexType(0.0, 0.0)));
    ok = ok && registerParameter(enabled_) && registerParameter(kernelFile_) && registerParameter(fftSize_);

    buildFFTs();

    return ok && Super::startup();
}

bool
LowPassFilter::loadKernel()
{
    static Logger::ProcLog log("loadKernel", getLog());

    std::string file = kernelFile_->getValue().c_str();

    if (file == "") LOGERROR << "No kernel file provided.  Using default!" << std::endl;

    std::ifstream input(file.c_str());

    std::string line;
    std::vector<float> alphas;
    float val;

    // Read in each kernel value for the low pass filter
    //
    while (std::getline(input, line)) {
        std::istringstream converter(line);
        if (!(converter >> val)) {
            LOGERROR << "Unable to load kernel for filter.  Bad value: " << line << std::endl;
            return false;
        } else {
            alphas.push_back(val);
        }
    }

    kernel_.reset(new VsipComplexVector(fftSize_->getValue(), ComplexType(0.0, 0.0)));

    for (size_t i = 0; i < alphas.size(); i++) { kernel_->put(i, ComplexType(alphas[i], 0.0)); }

    float scale;
    vsip::Index<1> maxIndex;
    scale = vsip::sqrt(vsip::maxmgsqval(*kernel_, maxIndex));

    *kernel_ /= scale;
    *kernel_ = (*fwdFFT_)(*kernel_);
    *kernel_ = vsip::conj(*kernel_);

    return true;
}

bool
LowPassFilter::shutdown()
{
    // Release memory and other resources here.
    //
    return Super::shutdown();
}

void
LowPassFilter::fftSizeChanged(const Parameter::PositiveIntValue& parameter)
{
    Logger::ProcLog log("fftSizeChanged", getLog());
    LOGINFO << "fftSize: " << parameter.getValue() << std::endl;
    buildFFTs();
}

void
LowPassFilter::buildFFTs()
{
    Logger::ProcLog log("buildFFTs", getLog());
    int fftSize = fftSize_->getValue();
    LOGINFO << "rebuilding FFTW with size " << fftSize << std::endl;

    fwdFFT_.reset(new FwdFFT(vsip::Domain<1>(fftSize), 1.0));
    invFFT_.reset(new InvFFT(vsip::Domain<1>(fftSize), 1.0 / fftSize));
    filtered_data_.reset(new VsipComplexVector(fftSize, ComplexType(0.0, 0.0)));

    loadKernel();
}

bool
LowPassFilter::processInput(const Messages::Video::Ref& msg)
{
    static Logger::ProcLog log("processInput", getLog());

    int msg_size = msg->getSize() / 2;

    // Create a new message to hold the output of what we do. Note that although we pass in the input message,
    // the new message does not contain any data.
    //
    Messages::Video::Ref out(Messages::Video::Make("LowPassFilter::processInput", msg));
    Messages::Video::Container& outputData(out->getData());

    // Fetch the FFT parameters. The number of windows is the number of FFTs we must execute in order to process
    // the filter span. Using ::ceil to sure that numWindows covers the entire message.
    //
    const int fftSize = fftSize_->getValue();
    const int numWindows = static_cast<int>(::ceil(static_cast<float>(msg_size) / fftSize));
    const int totalFFTSize = fftSize * numWindows;
    const int padding = totalFFTSize - msg_size;

    // Process the RX buffer as windows of fftSize.
    //
    for (int windowIndex = 0; windowIndex < numWindows; ++windowIndex) {
        int offset = windowIndex * fftSize;

        // Copy the appropriate portion of the input into the appropriate FFT buffer
        //
        for (int index = 0; index < fftSize; ++index) {
            filtered_data_->put(index, ComplexType(msg[2 * (offset + index)], msg[2 * (offset + index) + 1]));
        }

        // Transform input data into frequency space, correlate, and translate result back into time space.
        //
        (*filtered_data_) = (*invFFT_)((*kernel_) * (*fwdFFT_)(*filtered_data_));

        // Copy filtered data into output message.
        //
        int limit = fftSize;
        if (windowIndex == numWindows - 1) limit -= padding;

        for (int index = 0; index < limit; ++index) {
            outputData.push_back(Messages::Video::DatumType((*filtered_data_)(index).real()));
            outputData.push_back(Messages::Video::DatumType((*filtered_data_)(index).imag()));
        }

    } // end for each window

    // Send out on the default output device, and return the result to our Controller. NOTE: for multichannel
    // output, one must give a channel index to the send() method. Use getOutputChannelIndex() to obtain the
    // index for an output channel with a given name.
    //
    bool rc = send(out);
    LOGDEBUG << "rc: " << rc << std::endl;

    return rc;
}

void
LowPassFilter::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[LowPassFilter::kEnabled]) return Algorithm::FormatInfoValue("Disabled");
    return NULL;
}

// Factory function for the DLL that will create a new instance of the LowPassFilter class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
LowPassFilterMake(Controller& controller, Logger::Log& log)
{
    return new LowPassFilter(controller, log);
}
