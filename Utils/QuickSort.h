#ifndef UTILS_QUICKSORT_H // -*- C++ -*-
#define UTILS_QUICKSORT_H

#include <cstdlib> // for random()

namespace Utils {

/** Implementation of the in-place Quicksort algorithm. Orders the elements in a container in the range [low,
    high).

    \param low iterator to first element to order

    \param high iterator to first element to not order
*/
template <typename RandomAccessIterator>
void
QuickSort(RandomAccessIterator low, RandomAccessIterator high)
{
    using ValueType = typename std::iterator_traits<RandomAccessIterator>::value_type;

    // Randomly calculate a pivot to use for the partitioning.
    //
    ValueType pivot = low[::random() % (high - low)];

    // Walk the iterators towards each other, looking for out-of-order elements with respect to the pivot value.
    // We are looking for the point at which we can swap two elements.
    //
    RandomAccessIterator nl = low;
    RandomAccessIterator nh = high - 1;
    do {
        while (*nl < pivot) ++nl;
        while (*nh > pivot) --nh;

        if (nl <= nh) {
            // Swap the elements. NOTE: if iterators are the same, this is a slight waste, but we need to
            // increment them anyway.
            //
            ValueType tmp = *nl;
            *nl++ = *nh;
            *nh-- = tmp;
        }
    } while (nl <= nh);

    // If we need to, sort the partitions below and above the pivot.
    //
    if (nl < high) QuickSort(nl, high);
    if (nh > low) QuickSort(low, nh + 1);
}

} // end namespace Utils

#endif
