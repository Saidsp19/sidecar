#ifndef SIDECAR_ALGORITHMS_MATCHEDFILTER_TYPES_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MATCHEDFILTER_TYPES_H

#include <complex>

#include "ace/Message_Queue_T.h"

#include <vsip/signal.hpp>
#include <vsip/vector.hpp>

namespace SideCar {
namespace Algorithms {
namespace MatchedFilterUtils {

class WorkRequest;
class WorkerThreads;
class WorkRequestQueue;

using ComplexType = std::complex<float>;
using VsipComplexVector = vsip::Vector<ComplexType>;
using FwdFFT = vsip::Fft<vsip::Vector,ComplexType,ComplexType,vsip::fft_fwd>;
using InvFFT = vsip::Fft<vsip::Vector,ComplexType,ComplexType,vsip::fft_inv>;

}}}

#endif
