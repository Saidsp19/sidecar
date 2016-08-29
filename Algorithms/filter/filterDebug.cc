#include "ace/FILE_Connector.h"
#include "ace/Reactor.h"
#include "ace/Stream.h"

#include "IO/Readers.h"
#include "IO/FileWriterTask.h"
#include "IO/MessageManager.h"

#include "Logger/Log.h"
#include "Messages/Video.h"
#include "Parameter/Parameter.h"
#include "Utils/FilePath.h"

#include "boost/numeric/ublas/matrix.hpp"
#include "boost/numeric/ublas/io.hpp"
#include <vector>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vsip/vector.hpp>
#include <vsip/matrix.hpp>
#include <vsip/math.hpp>
#include <vsip/impl/signal-conv.hpp>
#include <vsip/initfin.hpp>
#include <vsip/support.hpp>
#include <vsip/signal.hpp>
#include <vsip/initfin.hpp>
#include <vsip/support.hpp>
#include <vsip/signal.hpp>
#include <vsip/math.hpp>
#include <complex>

using namespace SideCar::IO;
using namespace SideCar::Messages;
using namespace std;

using VIDEODATA = float;
using COMPLEXVIDEODATA = complex<VIDEODATA>;
using D2 = vsip::Domain<2>;
using D1 = vsip::Domain<1>;

// display a row of the matrix with tabs.
std::ostream & operator << (std::ostream & os, vsip::Vector<VIDEODATA> v)
{
    int idx = 0;
    int size = v.size();
    for (; idx < size; ++idx)
        os << v.get(idx) << '\t';
    return os;
}

std::ostream & operator << (std::ostream & os, vsip::Vector<COMPLEXVIDEODATA> v)
{
    int idx = 0;
    int size = v.size();
    for (; idx < size; ++idx) {
        COMPLEXVIDEODATA d = v.get(idx);
        char s = ((d.imag() < 0.0f) ? '-' : '+');
        os << d.real() << s << fabs(d.imag()) << "j\t";
    }
    return os;
}

// Prints out a matrix row-wise
std::ostream & operator << (std::ostream & os, vsip::Matrix<VIDEODATA> m)
{
    int idx = 0;
    int size = m.size(0); /* m.size(1) is the size of a dimension of the matrix */
    for(; idx < size; ++idx)
        os << (vsip::Vector<VIDEODATA>)m.row(idx) << std::endl;
    return os;
}

// Prints out a matrix row-wise
std::ostream & operator << (std::ostream & os, vsip::Matrix<COMPLEXVIDEODATA> m)
{
    int idx = 0;
    int size = m.size(0); /* m.size(1) is the size of a dimension of the matrix */
    for(; idx < size; ++idx)
        os << (vsip::Vector<COMPLEXVIDEODATA>)m.row(idx) << std::endl;
    return os;
}

vsip::Matrix<VIDEODATA>* readCSV (std::istream& is)
{
    std::string line, tok1;

    //read in the number of rows and columns
    if (!std::getline(is, line)) {
        cout << "bad csv file" << std::endl;
        abort();
    }
    std::istringstream iss(line);
    if (!std::getline(iss, tok1, ',')) {
        cout << "could not get number of rows" << std::endl;
        abort();
    }
    int rows = atoi(tok1.c_str());
    if (!std::getline(iss, tok1, ',')) {
        cout << "could not get number of rows" << std::endl;
        abort();
    }
    int cols = atoi(tok1.c_str());

    //create the matrix
    vsip::Matrix<VIDEODATA>* m = new vsip::Matrix<VIDEODATA>(rows, cols);

    for (int r = 0; r < rows; r++) {
        if (!std::getline(is, line)) {
            cout << "error reading line #" << r << std::endl;
            abort();
        }
        std::istringstream iss2(line);
        for (int c = 0; c < cols; c++) {
            if (!std::getline(iss2, tok1, ',')) {
                cout << "error read line #" << r << " col #" << c << std::endl;
                abort();
            }
            (*m)(r,c) = atoi(tok1.c_str());
        }
    }

    return m;
}

int main(int argc, const char* argv[])
{
    Logger::Log::Root().setPriorityLimit(Logger::Priority::kDebug);
    Logger::Log::Root().debug() << "this is a debug message" << std::endl;

    /*
      ifstream in ("test.csv", ios::in);
      if (!in) {
      cout << "unable to open file" << std::endl;
      abort();
      }

      vsip::Matrix<int>* kernel = readCSV(in);
      vsip::Matrix<int> input(kernel->size(0),10,1);
      vsip::Matrix<int> output(input.size(0)-kernel->size(0)+1, input.size(1)-kernel->size(1)+1);
	
      using conv_t = vsip::Convolution<vsip::const_Matrix, vsip::nonsym, vsip::support_min, int, 0, vsip::alg_space>;

      for (int c = 0; c < input.size(1); c += 2) {
      for (int r = 0; r < input.size(0); ++r) {
      input(r,c) = c;
      }
      }

      conv_t convolver (*kernel,
      vsip::Domain<2>(vsip::Domain<1>(input.size(0)),
      vsip::Domain<1>(input.size(1))), 1);

      convolver(input, output);

      cout << "kernel: " << kernel->size(0) << "x" << kernel->size(1) << std::endl << (*kernel) << std::endl
      << "input: " << std::endl << input << std::endl
      << "output: " << std::endl << output << std::endl;

      delete kernel;
    */

    cout << "reading in the image" << std::endl;
    ifstream in ("sample_pri.csv", ios::in);
    if (!in) {
        cout << "unable to open file" << std::endl;
        abort();
    }
    vsip::Matrix<VIDEODATA>* input = readCSV(in);
    //cout << "image:" << *input << std::endl;
    in.close();

    cout << "reading in the kernel" << std::endl;
    in.open("range_falling_edge.csv", ios::in);
    if (!in) {
        cout << "unable to open file" << std::endl;
        abort();
    }
    vsip::Matrix<VIDEODATA>* kernel = readCSV(in);
    cout << "kernel:" << std::endl << *kernel << std::endl;
    in.close();
	
    cout << "starting processing..." << std::endl;

    /*
      using FFT = vsip::Fft<vsip::const_Matrix, COMPLEXVIDEODATA, COMPLEXVIDEODATA, vsip::fft_fwd,
      vsip::by_reference>;
      using IFFT = vsip::Fft<vsip::const_Matrix, COMPLEXVIDEODATA, COMPLEXVIDEODATA, vsip::fft_inv,
      vsip::by_reference>;

      cout << "zero padding the kernel" << std::endl;
      vsip::Matrix<COMPLEXVIDEODATA> zeroPaddedKernel(input->size(0), input->size(1), COMPLEXVIDEODATA(0.0f,0.0f));
      vsip::Matrix<COMPLEXVIDEODATA> fft_image(input->size(0), input->size(1), COMPLEXVIDEODATA(0.0f,0.0f));	
      for (int r = 0; r < kernel->size(0); r++) {
      for (int c = 0; c < kernel->size(1); c++) {
      zeroPaddedKernel(r,c) = COMPLEXVIDEODATA((float)(*kernel)(r,c), 0.0f);
      }
      }

      cout << "converting the image from real to complex data" << std::endl;
      for (int r = 0; r < input->size(0); r++)
      for (int c = 0; c < input->size(1); c++)
      fft_image(r,c) = COMPLEXVIDEODATA((float)(*input)(r,c), 0.0f);
      //FFT fft (D2(D1(zeroPaddedKernel.size(0)), D1(zeroPaddedKernel.size(1))), 1);
      //IFFT ifft (D2(D1(zeroPaddedKernel.size(0)), D1(zeroPaddedKernel.size(1))), 1);

      */

    cout << "creating the fft structures" << std::endl;
    using FFT_r2c = vsip::Fft<vsip::const_Matrix, VIDEODATA, COMPLEXVIDEODATA, 1, vsip::by_reference, 1,
                              vsip::alg_time>;
    using FFT_c2c = vsip::Fft<vsip::const_Matrix, COMPLEXVIDEODATA, COMPLEXVIDEODATA, vsip::fft_fwd,
                              vsip::by_reference>;
    using IFFT_c2c = vsip::Fft<vsip::const_Matrix, COMPLEXVIDEODATA, COMPLEXVIDEODATA, vsip::fft_inv,
                               vsip::by_reference>;

    vsip::Matrix<COMPLEXVIDEODATA> zeroPaddedKernel(input->size(0), input->size(1), COMPLEXVIDEODATA(0.0f,0.0f));
    vsip::Matrix<COMPLEXVIDEODATA> fft_image(input->size(0), input->size(1), COMPLEXVIDEODATA(0.0f,0.0f));
    vsip::Matrix<COMPLEXVIDEODATA> fft_zeroPaddedKernel(input->size(0), input->size(1),
                                                        COMPLEXVIDEODATA(0.0f,0.0f));   
    FFT_r2c fft_r2c (D2(D1(zeroPaddedKernel.size(0)), D1(zeroPaddedKernel.size(1))), 1);
    FFT_c2c fft_c2c (D2(D1(zeroPaddedKernel.size(0)), D1(zeroPaddedKernel.size(1))), 1);
    IFFT_c2c ifft_c2c(D2(D1(zeroPaddedKernel.size(0)), D1(zeroPaddedKernel.size(1))), 1);

    cout << "zero padding the kernel" << std::endl;
    for (int r = 0; r < kernel->size(0); r++) {
        for (int c = 0; c < kernel->size(1); c++) {
            zeroPaddedKernel(r,c) = COMPLEXVIDEODATA((float)(*kernel)(r,c), 0.0f);
        }
    }
	
    cout << "converting the image from real to complex data" << std::endl;
    for (int r = 0; r < input->size(0); r++)
        for (int c = 0; c < input->size(1); c++)
            fft_image(r,c) = COMPLEXVIDEODATA((float)(*input)(r,c), 0.0f);

    cout << "ffting the kernel" << std::endl;
    fft_c2c(zeroPaddedKernel, fft_zeroPaddedKernel);
    //fft(zeroPaddedKernel);

    ofstream out ("fft_kernel.tab", ios::out);
    if (!out) {
        cout << "unable to create output file" << std::endl;
        abort();
    }
    out << fft_zeroPaddedKernel << std::endl;
    out.close();

    cout << "ffting the image" << std::endl;
    //fft_r2c(*input, fft_image);
    fft_c2c(fft_image);
		
    out.open("fft_image.tab", ios::out);
    if (!out) {
        cout << "unable to create output file" << std::endl;
        abort();
    }
    out << fft_image << std::endl;
    out.close();

    cout << "multiplication in the freq domain" << std::endl;
    fft_image *= fft_zeroPaddedKernel;
    //fft_image *= zeroPaddedKernel;

    out.open("fft_image_kernel.tab", ios::out);
    if (!out) {
        cout << "unable to create output file" << std::endl;
        abort();
    }
    out << fft_image << std::endl;
    out.close();

    cout << "ifft" << std::endl;
    ifft_c2c(fft_image);
    //ifft(fft_image);

    out.open("im_conv_kernel.tab", ios::out);
    if (!out) {
        cout << "unable to create output file" << std::endl;
        abort();
    }
    out << fft_image << std::endl;
    out.close();	
}
