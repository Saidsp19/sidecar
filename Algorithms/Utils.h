#ifndef SIDECAR_ALGORITHMS_UTILS_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_UTILS_H

#include <stddef.h>

class QString;

namespace SideCar {
namespace Algorithms {
namespace Utils {

extern bool normalizeSampleRanges(int& start, int& span, size_t maxSize);

} // end namespace Utils
} // end namespace Algorithms
} // end namespace SideCar

#endif
