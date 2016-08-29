#ifndef SIDECAR_GUI_SPECTRUM_DOUBLEVECTOR_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_DOUBLEVECTOR_H

#include <vector>

#include "FFTWAllocator.h"

namespace SideCar {
namespace GUI {
namespace Spectrum {

using DoubleVector = std::vector<double,FFTWAllocator<double> >;

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
