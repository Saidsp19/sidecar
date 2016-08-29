#ifndef UTILS_QUICKSELECT_H // -*- C++ -*-
#define UTILS_QUICKSELECT_H

#include <cstdlib>

#include <vector>

#include "Logger/Log.h"

namespace Utils {

/** Implementation of the in-place Quickselect algorithm. Freely manipulates the elements in a container in the
    range [low, high).

    \param low iterator to first element to order

    \param high iterator to first element to not order

    \param k the ordered statistic to return
*/
template <typename RandomAccessIterator> 
typename std::iterator_traits<RandomAccessIterator>::value_type
QuickSelect(RandomAccessIterator low, RandomAccessIterator high, size_t k)
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

	    // Swap the elements. NOTE: if iterators are the same, this is a slight waste.
	    //
	    ValueType tmp = *nl;
	    *nl++ = *nh;
	    *nh-- = tmp;
	}
    } while (nl <= nh);

    // See if the requested index is inside the lower partition.
    //
    size_t lowerSize = nl - low;
    if (k < lowerSize) {
	return QuickSelect(low, nl, k);
    }

    // See if the requested index is inside the upper partition.
    //
    k -= lowerSize;
    if (k) {
	return QuickSelect(nl, high, k);
    }

    // Return the found value.
    //
    return *nl;
}

} // end namespace Utils

#endif
