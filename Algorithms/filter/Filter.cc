#include "boost/bind.hpp"
#include <fstream>

#include "IO/MessageManager.h"
#include "Logger/Log.h"
#include "Messages/BinaryVideo.h"
#include "Messages/Extraction.h"
#include "Utils/Utils.h"

#include "Filter.h"

using namespace SideCar;
using namespace SideCar::Algorithms;

// display a row of the matrix with tabs.
std::ostream&
operator<<(std::ostream& os, Filter::DataVector v)
{
    int idx = 0;
    int size = v.size();
    for (; idx < size; ++idx) {
#ifndef USE_FAST_CONVOLUTION
        os << v.get(idx) << "\t";
#else
        Filter::COMPLEXFFTDATA d = v.get(idx);
        char s = ((d.imag() < 0.0f) ? '-' : '+');
        os << d.real() << s << fabs(d.imag()) << "j\t";
#endif
    }
    // os << v.get(idx) << '\t';
    return os;
}

// Prints out a matrix row-wise
std::ostream&
operator<<(std::ostream& os, Filter::DataMatrix m)
{
    // os << m.size(0) << "x" << m.size(1) << std::endl;
    for (int idx = 0; idx < (int)m.size(0); ++idx) os << m.row(idx) << std::endl;
    return os;
}

Filter::Filter(Controller& controller, Logger::Log& log) :
    Algorithm(controller, log), m_maxRangeBinValid(false),
    m_kernelCSVFilename(Parameter::ReadPathValue::Make("kernel", "CSV Kernel Path", ""))
//   "/export/home/sidecar/trunk/Algorithms/filter/range_rising_edge.csv"))
{
    m_kernelCSVFilename->connectChangedSignalTo(boost::bind(&Filter::kernelCSVFilenameChanged, this, _1));
}

bool
Filter::startup()
{
    registerProcessor<Filter, Messages::Video>(&Filter::process);
    loadKernel();
    return registerParameter(m_kernelCSVFilename) && Algorithm::startup();
}

bool
Filter::shutdown()
{
    static Logger::ProcLog log("shutdown", getLog());
    return true;
}

bool
Filter::reset()
{
    // start inserting on the first row of the buffer
    m_insertRow = 0;

    // delete the m_priData
    m_priData.reset();

    loadKernel();
    /*
    //create a 1x1 kernel with a value of 1 in the cell, this will do nothing
    //to the video
    DataMatrixPtr allPassKernel (new DataMatrix(1, 1, 1.0f));
    doInternalSetup (allPassKernel);
    resizePriData();
    */

    return true;
}

bool
Filter::process(const Messages::Video::Ref& inMsg)
{
    static Logger::ProcLog log("process", getLog());

    // lock the m_kernel (this is so it can't be updated while we're using it),
    // just return immediately (do not wait)

    // track the largest PRI buffer (in terms of number of range bins)
    if (((RANGEBIN)inMsg->size() > m_maxRangeBin) || (!m_maxRangeBinValid)) {
        m_maxRangeBin = (RANGEBIN)inMsg->size();
        m_maxRangeBinValid = true;
        LOGDEBUG << "assigning new max range = " << m_maxRangeBin << std::endl;
        resizePriData();
    }

    // insert new data from inMsg into the m_priData matrix
    for (RANGEBIN r = 0; r < (RANGEBIN)inMsg->size(); r++) { (*m_priData)(m_insertRow, r) = inMsg[r]; }
    // LOGDEBUG << "m_priData:" << std::endl << (*m_priData) << std::endl;

#ifndef USE_FAST_CONVOLUTION
    ////////////////////////////////////////////////////////////////////////
    // determine which wrapped kernel to use
    int kernelNum = m_wrappedKernels.size() - 1 - m_insertRow;
    DataMatrixPtr kernel = m_wrappedKernels[kernelNum];

    // convole the m_priData with the selected kernel
    ConvolveMatrix convolver(*kernel, D2(D1(m_priData->size(0)), D1(m_priData->size(1))), 1);
    convolver(*m_priData, *m_scratch);

    // create the output message/matrix
    Messages::Video::Ref outMsg(Messages::Video::Make(getName(), inMsg));
    outMsg->resize(inMsg->size());
    RANGEBIN zeroPad = kernel->size(1) / 2;
    for (RANGEBIN c = 0; c < (RANGEBIN)outMsg->size(); c++) {
        if ((c < zeroPad) || (c >= m_scratch->size(1) + zeroPad))
            outMsg[c] = 0;
        else
            outMsg[c] = (VIDEODATA)(*m_scratch)(0, c - zeroPad);
    }

    //////////////////////////////////////////////////////////////////////
#else
    /*
      static int count = 0;

      ofstream out;
      if (count == 0)
      out.open("filter_pridata.tab", ios::out);
      else
      out.open("filter_pridata.tab", ios::app);
      for (int i = 0; i < (int)inMsg->size(); i++)
      out << inMsg[i] << "\t";
      out << std::endl;
      out.close();

      if (count == 0) {
      out.open("filter_kernel.tab", ios::out);
      out << *m_kernel;
      out.close();

      out.open("filter_fftkernel.tab", ios::out);
      out << *m_fftKernel;
      out.close();
      }

      count++;
    */

    ////////////////////////////////////////////////////////////////////////
    (*m_fft)(*m_priData, *m_scratch);
    (*m_scratch) *= (*m_fftKernel);
    (*m_ifft)(*m_scratch);
    /*
  ////TODO: remove this code, for debug purposes only
  char fn[128];
  sprintf(fn, "filter_pridata%02i.tab", count);
  out.open(fn, ios::out);
  out << *m_priData;
  out.close();

  sprintf(fn, "filter_fftpridata%02i.tab", count);
  out.open(fn, ios::out);
  out << *m_scratch;
  out.close();

  (*m_scratch) *= (*m_fftKernel);

  sprintf(fn, "filter_mult%02i.tab", count);
  out.open(fn, ios::out);
  out << *m_scratch;
  out.close();

  (*m_ifft)(*m_scratch);

  sprintf(fn, "filter_ifft%02i.tab", count);
  out.open(fn, ios::out);
  out << *m_scratch;
  out.close();

  ////end remove
  */

    Messages::Video::Ref outMsg(Messages::Video::Make(getName(), inMsg));
    outMsg->resize(m_priData->size(1));

    RANGEBIN offset = m_kernel->size(1) / 2;
    for (RANGEBIN c = 0; c < (RANGEBIN)m_priData->size(1); c++) {
        outMsg[c] = (VIDEODATA)((*m_scratch)(0, (c + offset) % m_priData->size(1))).real();
    }

    ////////////////////////////////////////////////////////////////////////
#endif

    // increment to the next line (so that the data is inserted in a fifo style)
    m_insertRow = (m_insertRow + 1) % m_priData->size(0);

    // send the new data
    bool rc = send(outMsg);
    return rc;
}

extern "C" ACE_Svc_Export Algorithm*
FilterMake(Controller& controller, Logger::Log& log)
{
    return new Filter(controller, log);
}

void
Filter::resizePriData()
{
    static Logger::ProcLog log("resizePriData", getLog());

    if (!m_maxRangeBinValid) return;

        // the m_priData should always be of size rows x cols (as follows)
#ifndef USE_FAST_CONVOLUTION
    PRI_COUNT rows = (PRI_COUNT)m_wrappedKernels[0]->size(0);
#else
    PRI_COUNT rows = (PRI_COUNT)m_kernel->size(0);
#endif

    RANGEBIN cols = m_maxRangeBin;

    if (!m_priData) {
        LOGDEBUG << "creating data (" << rows << "x" << cols << ")" << std::endl;
        m_priData = DataMatrixPtr(new DataMatrix(rows, cols, 0));
        // LOGDEBUG << (*m_priData) << std::endl;

    } else if ((rows != (PRI_COUNT)m_priData->size(0))) {
        // if the number of required rows has changed, then the kernel has
        //  changed just discard all data
        LOGDEBUG << "resizing the number of rows (" << rows << "x" << cols << ")" << std::endl;
        m_priData = DataMatrixPtr(new DataMatrix(rows, cols, 0));

    } else if ((cols != (RANGEBIN)m_priData->size(1))) {
        // if only the columns are mismatched, copy the data (the number of cols
        //  required can change as the pri length changes -- e.g., as the
        //  result of PRF stagger)
        LOGDEBUG << "resizing the number of cols (" << rows << "x" << cols << ")" << std::endl;
        DataMatrixPtr newData(new DataMatrix(rows, cols, 0));
        RANGEBIN copyCols = std::min(cols, (RANGEBIN)m_priData->size(1));
        (*newData)(D2(D1(rows), D1(copyCols))) = (*m_priData)(D2(D1(rows), D1(copyCols)));
        m_priData = newData;
    }

#ifndef USE_FAST_CONVOLUTION
    // resize the scratch matrix as appropriate
    vsip::length_type outputSize = m_priData->size(1) - m_wrappedKernels[0]->size(1) + 1;
    m_scratch = DataMatrixPtr(new DataMatrix(1, outputSize));
#else
    // initialize the fft and ifft structures. also, pre-calcualte the
    //  fft of the kernel.
    LOGDEBUG << "initializing the fft...";
    m_fft = FFTPtr(new FFT(D2(D1(rows), D1(cols)), 1.0f));

    LOGDEBUG << "done" << std::endl << "initializing the inverse fft...";
    m_ifft = IFFTPtr(new IFFT(D2(D1(rows), D1(cols)), 1.0f / (FFTDATA)(rows * cols)));

    DataMatrix zeroPaddedKernel(rows, cols, COMPLEXFFTDATA(0.0f, 0.0f));
    zeroPaddedKernel(D2(D1(m_kernel->size(0)), D1(m_kernel->size(1)))) = *m_kernel;

    m_fftKernel = DataMatrixPtr(new DataMatrix(rows, cols));
    m_scratch = DataMatrixPtr(new DataMatrix(rows, cols));

    LOGDEBUG << "done" << std::endl << "calculating the fft of the kernel...";
    (*m_fft)(zeroPaddedKernel, *m_fftKernel);

    LOGDEBUG << "done" << std::endl;

    // LOGDEBUG << "kernel" << std::endl << zeroPaddedKernel << std::endl
    //             << "fft(kernel)" << std::endl << *m_fftKernel << std::endl;
#endif
}

void
Filter::kernelCSVFilenameChanged(const Parameter::ReadPathValue& value)
{
    loadKernel();
}

void
Filter::loadKernel()
{
    static Logger::ProcLog log("kernelCSVFilenameChanged", getLog());
    LOGINFO << "loading kernel file " << m_kernelCSVFilename->getValue() << std::endl;

    DataMatrixPtr newKernel;

    std::ifstream in(m_kernelCSVFilename->getValue().c_str(), ios::in);
    if (!in) {
        LOGWARNING << "unable to open file \"" << m_kernelCSVFilename->getValue()
                   << "\", defaulting to a 1x1 all pass kernel" << std::endl;
        newKernel = DataMatrixPtr(new DataMatrix(1, 1, 1.0f));
    } else {
        newKernel = readCSV(in);
    }

    doInternalSetup(newKernel);
    resizePriData();

    LOGDEBUG << "done" << std::endl;
}

void
Filter::doInternalSetup(DataMatrixPtr kernel)
{
    static Logger::ProcLog log("createWrappedKernels", getLog());

#ifndef USE_FAST_CONVOLUTION
    ////////////////////////////////////////////////////////////////////////
    // delete any existing kernels
    m_wrappedKernels.clear();

    // add the original kernel to the list
    m_wrappedKernels.push_back(kernel);

    for (vsip::index_type i = 1; i < kernel->size(0); i++) {
        // rotate the kernel by one row
        DataMatrixPtr rotated1Row(new DataMatrix(kernel->size(0), kernel->size(1)));
        // assume rotated1Row and kernel have dimensions [0:M] x [0:N]
        // rotated1Row(0:M-1,:) = kernel(1:M,:)
        (*rotated1Row)(D2(D1(0, 1, kernel->size(0) - 1), D1(kernel->size(1)))) =
            (*kernel)(D2(D1(1, 1, kernel->size(0) - 1), D1(kernel->size(1))));
        // rotated1Row(M,:) = kernel(0,:)
        (*rotated1Row)(D2(D1(kernel->size(0) - 1, 1, 1), D1(kernel->size(1)))) =
            (*kernel)(D2(D1(0, 1, 1), D1(kernel->size(1))));

        m_wrappedKernels.push_back(rotated1Row);
        kernel = rotated1Row;
    }

    for (int j = 0; j < (int)m_wrappedKernels.size(); j++) {
        LOGDEBUG << "kernel #" << j << std::endl << *(m_wrappedKernels[j]) << std::endl;
    }
    ////////////////////////////////////////////////////////////////////////
#else
    ////////////////////////////////////////////////////////////////////////
    // just save the kernel, nothing to do
    m_kernel = kernel;

    ////////////////////////////////////////////////////////////////////////
#endif
}

Filter::DataMatrixPtr
Filter::readCSV(std::istream& is)
{
    static Logger::ProcLog log("readCSV", getLog());
    std::string line, tok1;

    // read in the number of rows and columns
    if (!std::getline(is, line)) {
        LOGERROR << "bad csv file" << std::endl;
        abort();
    }

    std::istringstream iss(line);
    if (!std::getline(iss, tok1, ',')) {
        LOGERROR << "could not get number of rows" << std::endl;
        abort();
    }

    int rows = atoi(tok1.c_str());
    if (!std::getline(iss, tok1, ',')) {
        LOGERROR << "could not get number of rows" << std::endl;
        abort();
    }

    int cols = atoi(tok1.c_str());

    // create the matrix
    DataMatrixPtr m = DataMatrixPtr(new DataMatrix(rows, cols));

    for (int r = 0; r < rows; r++) {
        if (!std::getline(is, line)) {
            LOGERROR << "error reading line #" << r << std::endl;
            abort();
        }

        std::istringstream iss2(line);
        for (int c = 0; c < cols; c++) {
            if (!std::getline(iss2, tok1, ',')) {
                LOGERROR << "error read line #" << r << " col #" << c << std::endl;
                abort();
            }
#ifndef USE_FAST_CONVOLUTION
            (*m)(r, c) = atof(tok1.c_str());
#else
            (*m)(r, c) = COMPLEXFFTDATA(atof(tok1.c_str()), 0.0f);
#endif
        }
    }

    return m;
}
