#ifndef SIDECAR_UTILS_VSIPVECTOR_H // -*- C++ -*-
#define SIDECAR_UTILS_VSIPVECTOR_H

#include <vsip/math.hpp>
#include <vsip/vector.hpp>

namespace SideCar {

/** Provides a simple wrapper around std::vector-like data structures. T needs to have T.size() and T[0]
    operators. DatumType needs to match the size of entries in T.
*/
template <class T, class DatumType = typename T::value_type>
class VsipVector {
public:
    /** Constructor. Wraps T.size() entries, starting with T[0].
     */
    VsipVector(T& base) : m_(vsip::Domain<1>(base.size()), &base[0]), v(m_) {}

    /** Wraps size entries, starting with T[0].
     */
    VsipVector(T& base, size_t size) : m_(vsip::Domain<1>(size), &base[0]), v(m_) {}

    /** Wraps T[start] to T[stop].

        \param start initial index

        \param stop final index
    */
    VsipVector(T& base, size_t start, size_t stop) : m_(vsip::Domain<1>(stop - start + 1), &base[start]), v(m_) {}

    /** Set whether VSIPL will use existing data found in the external array given in the constructor.

        \param read if true VSIPL will use existing data
    */
    void admit(bool read) { m_.admit(read); }

    /** Set whether VSIPL will flush any data from internal storage to the external array given in the
        constructor.

        \param read if true VSIPL will flush data to the external array
    */
    void release(bool flush) { m_.release(flush); }

private:
    vsip::Dense<1, DatumType> m_;

public:
    vsip::Vector<DatumType> v;
};

} // namespace SideCar

/** \file
 */

#endif
