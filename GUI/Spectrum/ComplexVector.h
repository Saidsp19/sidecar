#ifndef SIDECAR_GUI_SPECTRUM_COMPLEXVECTOR_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_COMPLEXVECTOR_H

#include <complex>
#include <vector>

#include "FFTWAllocator.h"

namespace SideCar {
namespace GUI {
namespace Spectrum {

using Complex = std::complex<float>;
using ComplexVector = std::vector<Complex,FFTWAllocator<Complex> >;

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
