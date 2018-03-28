#ifndef BUFFER_H
#define BUFFER_H

#include <cmath>
#include <limits>
#include <ostream>

#include <vsip/vector.hpp>

#include "ModelView.h"

template <class, class, bool>
class BufferRowBase;
template <class T>
class BufferRow;
template <class T>
class const_BufferRow;
template <class T>
class BufferCol;
template <class T>
class BufferMat;

/**
   \brief A matrix class that supports VSIPL++ subviews.

   The Buffer class and its friend classes BufferRow, BufferCol, and BufferMat together provide a convenient framework
   for creating a 2-dimensional array of data that is accessible using VSIPL++.

   As shown in \ref fig1 "Figure 1", the Buffer is defined by two matrix sizes; the data matrix is padded such that it
   may be convolved with a smaller matrix up to a maximum specified size.  Given this maximum window size, the padding
   allows convolving arbitrary subwindows of the Buffer without fear of accessing undefined memory.  Also, the data rows
   are "wrapped" so that the Buffer may be used as a rolling window.

   \anchor fig1
   \image html conceptual_layout.png "Figure 1:  Conceptual layout"

   \ref fig2 "Figure 2" shows the underlying memory layout which supports the conceptual model.  The end user should
   understand two important decisions which were made.  First, a single space is used to buffer both ends of the data;
   this conserves memory, but it could also cause strange side effects if the user writes to this area (only possible
   with the BufferMat view).  Second, wrapping of the rows is achieved by copying the first few rows to the end of the
   matrix; in either location, updates to the first few rows are only copied to the other location when the view is
   flushed; this copy-on-flush policy allows the use of unmodified VSIPL++ view classes.

   \anchor fig2
   \image html mem_layout.png "Figure 2:  Physical layout"
*/
template <class T>
class Buffer : public BufferModel {
    friend class BufferRowBase<T, vsip::Vector<T>, true>;
    friend class BufferRowBase<T, vsip::const_Vector<T>, false>;
    friend class BufferRow<T>;
    friend class BufferCol<T>;
    friend class BufferMat<T>;

public:
    //! Class for viewing rows in this Buffer
    using Row = BufferRow<T>;
    //! Class for viewing rows in this Buffer
    using const_Row = const_BufferRow<T>;
    //! Class for viewing columns in this Buffer
    using Col = BufferCol<T>;
    //! Long form of Col
    using Column = BufferCol<T>;
    //! Class for viewing submatrices in this Buffer
    using Mat = BufferMat<T>;
    //! Long form of Mat
    using Matrix = BufferMat<T>;

public:
    /**
       \brief Constructs the Buffer object.

       Does not initialize the Buffer memory.  Allocates internal buffer memory if an external memory location is not
       passed.

       \param dr Number of data rows

       \param dc Number of data columns

       \param wr Maximum number of window rows

       \param wc Maximum number of window columns

       \param mem Externally-allocated memory, of minimum size (dr+wr)*(dc+wc)
    */
    Buffer(size_t dr, size_t dc, size_t wr = 0, size_t wc = 0, T* mem = 0);
    /**
       \brief Deletes the memory buffer if it was created internally.
    */
    ~Buffer();
    void print(std::ostream& os) const;

    // Data section
    //
    /**
       \brief Clear the data area

       Fills the data area with normally distributed random numbers.

       \param mean Mean of the distribution

       \param stddev Standard deviation
    */
    void clearData(T mean = 0, T stddev = 0);
    /**

     * \return Number of rows allocated for data
     */
    inline size_t getDataRows() const { return dr; }
    /**

     * \return Number of columns allocated for data
     */
    inline size_t getDataCols() const { return dc; }
    /**

     * \param row Not bounds checked

     * \param col Not bounds checked

     * \return the value at (row, col)
     */
    inline T get(size_t row, size_t col) const { return buf[row * cols + col]; }
    /**
     * \brief A "safe" get.

     * \param row Bounds checked

     * \param col Bounds checked

     * \return the value at (row, col)
     */
    inline T sget(size_t row, size_t col) const
    {
        assert(row < rows);
        assert(col < cols);
        return buf[row * cols + col];
    }

    // Window section
    //
    /**
       \brief Clear the window buffer area

       Fills the window buffer area with normally distributed random numbers.

       \param mean Mean of the distribution

       \param stddev Standard deviation
    */
    void clearWindow(T mean = 0, T stddev = 0);
    /**

     * \return Number of rows allocated for windowing
     */
    inline size_t getWindowRows() const { return wr; }
    /**

     * \return Number of columns allocated for windowing
     */
    inline size_t getWindowCols() const { return wc; }
    /**
       Resizes the Buffer to support a larger window. Preserves the data section, but does not initialize the window
       buffer area.
    */
    void resizeWindow(size_t wr = 0, size_t wc = 0);

private:
    // Internal data structures
    //
    size_t dr, dc;     // data rows and columns
    size_t wr, wc;     // window rows and columns
    size_t rows, cols; // rows=dr+wr; cols=dc+wc
    size_t length;     // = rows*cols
    T* buf;
    // int *rowguard; // indicates whether a row is "new" or "old"
    bool internal_mem; // Indicates that buf was an internally allocated buffer
};

template <class T>
std::ostream&
operator<<(std::ostream& os, const Buffer<T>& b)
{
    b.print(os);
    return os;
}

// assumes all values are hex (0-15)
template <>
void
Buffer<char>::print(std::ostream& os) const
{
    // set hex output mode
    os << std::hex;

    for (size_t i = 0; i < rows; i++) {
        size_t offset = i * cols;
        for (size_t j = 0; j < cols; j++) { os << (int)buf[offset + j]; }
        os << std::endl;
    }

    // restore decimal mode
    os << std::dec;

    return;
}

template <class T>
Buffer<T>::Buffer(size_t dr, size_t dc, size_t wr, size_t wc, T* mem) :
    dr(dr), dc(dc), wr(wr), wc(wc), rows(dr + wr), cols(dc + wc), length(rows * cols), buf(mem)
{
    if (!mem) {
        internal_mem = true;
        buf = new T[length];
    } else {
        internal_mem = false;
    }
}

template <class T>
void
Buffer<T>::clearData(T mean, T stddev)
{
    if (stddev) // Generate gaussian noise using the Box-Muller transform
    {
        size_t offset = 0;
        for (size_t row = 0; row < rows; row++) {
            for (size_t i = 0; i < dc; i++) {
                double x1, x2;
                double r = 0;
                do {
                    x1 = 2 * double(rand()) / RAND_MAX - 1;
                    x2 = 2 * double(rand()) / RAND_MAX - 1;
                    r = x1 * x1 + x2 * x2;
                } while (r == 0 || r >= 1);
                r = sqrt((-2 * log(r)) / r);

                double val = mean + stddev * x1 * r;
                if (val > std::numeric_limits<T>::max()) {
                    buf[offset + i] = std::numeric_limits<T>::max();
                } else if (val < std::numeric_limits<T>::min()) {
                    buf[offset + i] = std::numeric_limits<T>::min();
                } else {
                    buf[offset + i] = (T)val;
                }
                i++;
                if (i < dc) {
                    val = mean + stddev * x2 * r;
                    if (val > std::numeric_limits<T>::max()) {
                        buf[offset + i] = std::numeric_limits<T>::max();
                    } else if (val < std::numeric_limits<T>::min()) {
                        buf[offset + i] = std::numeric_limits<T>::min();
                    } else {
                        buf[offset + i] = (T)val;
                    }
                }
            }
            offset += cols;
        }
    } else // Simple fill
    {
        size_t offset = 0;
        for (size_t row = 0; row < rows; row++) {
            for (size_t i = 0; i < dc; i++) { buf[offset + i] = mean; }
            offset += cols;
        }
    }
}

template <class T>
void
Buffer<T>::clearWindow(T mean, T stddev)
{
    if (stddev) // Generate gaussian noise using the Box-Muller transform
    {
        size_t offset = dc;
        for (size_t row = 0; row < rows; row++) {
            for (size_t i = 0; i < wc; i++) {
                double x1, x2;
                double r = 0;
                do {
                    x1 = 2 * double(rand()) / RAND_MAX - 1;
                    x2 = 2 * double(rand()) / RAND_MAX - 1;
                    r = x1 * x1 + x2 * x2;
                } while (r == 0 || r >= 1);
                r = sqrt((-2 * log(r)) / r);
                double val = mean + stddev * x1 * r;
                if (val > std::numeric_limits<T>::max()) {
                    buf[offset + i] = std::numeric_limits<T>::max();
                } else if (val < std::numeric_limits<T>::min()) {
                    buf[offset + i] = std::numeric_limits<T>::min();
                } else {
                    buf[offset + i] = (T)val;
                }
                i++;
                if (i < dc) {
                    val = mean + stddev * x2 * r;
                    if (val > std::numeric_limits<T>::max()) {
                        buf[offset + i] = std::numeric_limits<T>::max();
                    } else if (val < std::numeric_limits<T>::min()) {
                        buf[offset + i] = std::numeric_limits<T>::min();
                    } else {
                        buf[offset + i] = (T)val;
                    }
                }
            }
            offset += cols;
        }
    } else // Simple fill
    {
        size_t offset = dc;
        for (size_t row = 0; row < rows; row++) {
            for (size_t i = 0; i < wc; i++) { buf[offset + i] = mean; }
            offset += cols;
        }
    }
}

template <class T>
void
Buffer<T>::resizeWindow(size_t newwr, size_t newwc)
{
    if (wr < newwr || wc < newwc) {
        assert(internal_mem); // need to request more memory from the originating source...

        // stop all registered views
        std::list<BufferView*>::iterator view;
        std::list<BufferView*>::const_iterator stop = views.end();
        for (view = views.begin(); view != stop; view++) { (*view)->pause(); }

        // allocate new memory
        int newrows = dr + newwr;
        int newcols = dc + newwc;
        size_t newlength = newrows + newcols;
        T* newbuf = new T[newlength];

        // copy the data over
        size_t offset = 0;
        size_t newoffset = 0;
        for (size_t row = 0; row < dr; row++) // copy over the main data section
        {
            for (size_t col = 0; col < dc; col++) { newbuf[newoffset + col] = buf[offset + col]; }
            offset += cols;
            newoffset += newcols;
        }
        offset = 0;
        for (size_t row = 0; row < wr; row++) // copy over the data wrap section
        {
            for (size_t col = 0; col < dc; col++) { newbuf[newoffset + col] = buf[offset + col]; }
            offset += cols;
            newoffset += newcols;
        }

        // make the switch
        delete[](buf);
        buf = newbuf;
        wr = newwr;
        wc = newwc;
        rows = newrows;
        cols = newcols;
        length = newlength;

        // restart all registered views
        for (view = views.begin(); view != stop; view++) { (*view)->resume(); }
    }
}

template <class T>
Buffer<T>::~Buffer()
{
    if (internal_mem) delete[](buf);
}

/** \file
 */

#endif
