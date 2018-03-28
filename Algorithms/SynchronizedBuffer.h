#ifndef SIDECAR_ALGORITHMS_SYNCHRONIZEDBUFFER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_SYNCHRONIZEDBUFFER_H

#include "Algorithms/PastBuffer.h"

namespace SideCar {
namespace Algorithms {

template <typename T>
class SynchronizedBuffer : public PastBuffer<T> {
public:
    using Super = PastBuffer<T>;

    SynchronizedBuffer(size_t capacity) : PastBuffer<T>(capacity) {}

    typename T::Ref find(uint32_t key)
    {
        while (!Super::empty()) {
            uint32_t oldest = Super::back()->getSequenceCounter();
            if (oldest > key) {
                break;
            } else if (oldest == key) {
                typename T::Ref found = Super::back();
                Super::pop_back();
                return found;
            }
            Super::pop_back();
        }

        return typename T::Ref();
    }
};

} // end namespace Algorithms
} // end namespace SideCar

#endif
