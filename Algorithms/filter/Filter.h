#ifndef SIDECAR_ALGORITHMS_FILTER_H	// -*- C++ -*-
#define SIDECAR_ALGORITHMS_FILTER_H

#include "boost/shared_ptr.hpp"
#include <istream>
#include <complex>
#include <vector>

#define VSIP_IMPL_FFTW3 1
#include <vsip/vector.hpp>
#include <vsip/math.hpp>
#include <vsip/signal.hpp>

#include "Algorithms/Algorithm.h"
#include "Algorithms/extractWithCentroiding/ImageDataTypes.h"
#include "Messages/PRIMessage.h"
#include "Parameter/Parameter.h"

#define USE_FAST_CONVOLUTION

namespace SideCar {
namespace Algorithms {

/** Filter algorithm.
 */

class Filter : public Algorithm
{
    //Generally speaking, the filter class updates the m_priData
    //  matrix in a ring buffer (fifo) that is dynamically sized
    //  to match the kernel.  if USE_FAST_CONVOLUTION is not defined,
    //  the convolution is done in the time domain.  because the m_priData
    //  is "wrapped", the convolution is done with a similarlly wrapped kernel.
    //  if USE_FAST_CONVOLUTION is defined, then the convolution
    //  is done in the frequency domain via multiplication. since this
    //  is actually circular convolution, the wrapping of the m_priData
    //  is irrelevant.

    //Notes on boundaries:
    // 1) USE_FAST_CONVOLUTION not defined: only valid data is reported. the
    //    output vector is the same size as the largest input pri, but zero
    //    padded.
    // 2) USE_FAST_CONVOLUTION is defined: the output vector is the same size
    //    as the largest pri. however, because circular convolution is used,
    //    the first and last portions of the vector (depending on the number of
    //    columns in the kernel) depend on the last and first (respectively)
    //    portions of the pri data.
public:

    Filter(Controller& controller, Logger::Log& log);

    void setKernelFilePath (const std::string& value)
	{ m_kernelCSVFilename->setValue(value); }

    bool startup();

    bool shutdown();

    bool reset();

    RANGEBIN m_maxRangeBin;
    bool m_maxRangeBinValid;

    using VideoMatrix = vsip::Matrix<VIDEODATA>;
    using VideoMatrixPtr = boost::shared_ptr<VideoMatrix>;
    using D2 = vsip::Domain<2>;
    using D1 = vsip::Domain<1>;
    using VideoDense = vsip::Dense<2, VIDEODATA>;
    using VideoDensePtr = boost::shared_ptr<VideoDense>;

#ifndef USE_FAST_CONVOLUTION
public :
    using ConvolveMatrix = vsip::Convolution<vsip::const_Matrix, vsip::nonsym, vsip::support_min, float, 0,
                                             vsip::alg_space>;

    using DataVector = vsip::Vector<float>;
    using DataMatrix = vsip::Matrix<float>;
    using DataMatrixPtr = boost::shared_ptr<DataMatrix>;
    using DataMatrixPtrVector = std::vector<DataMatrixPtr>;

private :
    DataMatrixPtrVector m_wrappedKernels;
    DataMatrixPtr m_scratch;

#else
public :
    using FFTDATA = float;
    using COMPLEXFFTDATA = std::complex<FFTDATA>;
    using DataVector = vsip::Vector<COMPLEXFFTDATA>;
    using DataMatrix = vsip::Matrix<COMPLEXFFTDATA>;
    using DataMatrixPtr = boost::shared_ptr<DataMatrix>;
    using FFT = vsip::Fft<vsip::const_Matrix, COMPLEXFFTDATA, COMPLEXFFTDATA, vsip::fft_fwd, vsip::by_reference>;
    using IFFT = vsip::Fft<vsip::const_Matrix, COMPLEXFFTDATA, COMPLEXFFTDATA, vsip::fft_inv, vsip::by_reference>;
    using FFTPtr = boost::shared_ptr<FFT>;
    using IFFTPtr = boost::shared_ptr<IFFT>;

private :
    DataMatrixPtr m_kernel;
    DataMatrixPtr m_fftKernel;
    DataMatrixPtr m_scratch;
    FFTPtr m_fft;
    IFFTPtr m_ifft;

#endif

private :
    bool process(const Messages::Video::Ref& inMsg);

    void loadKernel();
    void kernelCSVFilenameChanged(const Parameter::ReadPathValue& value);
    DataMatrixPtr readCSV(std::istream& is);
    void resizePriData();
    void doInternalSetup(DataMatrixPtr kernel);

    Parameter::ReadPathValue::Ref m_kernelCSVFilename;
    DataMatrixPtr m_priData;
    PRI_COUNT m_insertRow;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
