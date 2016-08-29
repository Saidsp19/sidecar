#ifndef SIDECAR_GUI_SPECTRUM_FFTWALLOCATOR_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_FFTWALLOCATOR_H

#include <limits>
#include <memory>

#include <fftw3.h>

namespace SideCar {
namespace GUI {
namespace Spectrum {

/** STL allocator implementation that uses the FFTW library for low-level memory allocation. The FFTW routines
    guarantee proper alignment for SSE and Altivec array processing.
*/
template <typename T>
class FFTWAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <class U>
    struct rebind {
	using other = FFTWAllocator<U>;
    };

    
    /** Convert a reference to a value into a pointer to same.

        \param value the reference to convert

        \return pointer
    */
    pointer address (reference value) const { return &value; }
    
    /** Convert a const reference to a value into a const pointer to same.

        \param value the reference to convert

        \return pointer
    */
    const_pointer address (const_reference value) const { return &value; }

    /** Constructor. Does nothing.
     */
    FFTWAllocator() throw() {}

    /** Copy constructor. Does nothing.

        \param copy object to copy
    */
    FFTWAllocator(const FFTWAllocator& copy) throw() {}

    /** Conversion copy constructor. Does nothing.

        \param copy object to copy
    */
    template <class U>
    FFTWAllocator(const FFTWAllocator<U>& copy) throw() {}

    /** Destructor. Does nothing.
     */
    ~FFTWAllocator() throw() {}

    /** Obtain the size of the biggest object that may be allocated by this allocator.

        \return 
    */
    size_type max_size () const throw()
	{ return std::numeric_limits<std::size_t>::max() / sizeof(T); }

    /** Allocate space for an object. NOTE: although this returns a pointer of type T, this routine does not
        return a constructed object.

        \param n size of the object

        \param locationaddress of space to use for allocation if not NULL
        (ignored)

        \return address of allocated space
    */
    pointer allocate(size_type n, const void* location = 0)
	{ return (pointer)(::fftw_malloc(n * sizeof(T))); }

    /** Construct a new T object in a given memory block.

        \param p memory block to use

        \param value a T instance to copy from
    */
    void construct(pointer p, const T& value)
	{ new((void*)(p))T(value); }

    /** Invoke the destructor for a given object. NOTE: does not release the memory holding the object.

        \param p pointer to the object to destroy
    */
    void destroy (pointer p)
	{ p->~T(); }

    /** Release the memory used to hold an object, obtained from an earlier allocate() call.

        \param p pointer to the memory to release

        \param num number of T instances in the memory (ignored)
    */
    void deallocate(pointer p, size_type num)
	{ ::fftw_free((void*)(p)); }
};

/** Equality comparison operator for FFTWAllocator types. Since these allocators have no state, they are all
    equal to one another.

    \param a first object

    \param b second object

    \return always TRUE
*/
template <class T1, class T2> bool
operator==(const FFTWAllocator<T1>& a, const FFTWAllocator<T2>& b) throw()
{
    return true;
}

/** Inequality comparison operator for FFTWAllocator types. Since these allocators have no state, they are all
    equal to one another.

    \param a first object

    \param b second object

    \return always FALSE
*/
template <class T1, class T2> bool
operator!=(const FFTWAllocator<T1>&, const FFTWAllocator<T2>&) throw()
{
    return false;
}

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
