#ifndef BUFFER_ROW_H
#define BUFFER_ROW_H

#include "ModelView.h"
#include <vsip/math.hpp>
#include <vsip/vector.hpp>

/**
   Internal class for use by BufferRow and const_BufferRow.

   \param T must match Buffer<T>

   \param VectorT choice of vsip::Vector<T> or vsip::const_Vector<T>

   \param WritableF indicates whether to flush the wrap row
*/
template <class T, class VectorT, bool WritableF>
class BufferRowBase : private BufferView {
public:
    /**
       \brief View one row of a Buffer

       The BufferRow is a subview of Buffer

       \param b Buffer to view

       \param row Which row to view

       \param flush Whether this view should change the underlying buffer
    */
    BufferRowBase(Buffer<T>& b, size_t row = 0);
    ~BufferRowBase();

    /**
       \brief Change which row to view

       \param Index of the new row to view
    */
    void setRow(size_t row = 0);
    /**

       \return Index of the row which is being viewed
    */
    inline size_t getRow() { return rowBase; }

    /**
       \brief Increment the view index by one.
    */
    inline void operator++(int) { setRow(row + 1); }

protected:
    Buffer<T>& buf;
    vsip::Dense<1, T> dense;
    void internalFlush(size_t row, bool rebind);

public:
    /**
       \brief The vsip::Vector which is mapped to this row

       This member is made public for ease of use; any operations on it will use the vsip::Vector operator overloads.
    */
    VectorT v;

protected:
    inline T* bufIndex(size_t r) { return buf.buf + buf.cols * r; }
    size_t rowBase; // the row the user selected, before the modulo operation
    size_t row;     // which row we are revealing

private:
    // BufferView interface
    //
    void pause() { internalFlush(row, false); }
    void resume()
    {
        dense.rebind(bufIndex(row));
        dense.admit(true);
    }
};

template <class T, class VectorT, bool WritableF>
BufferRowBase<T, VectorT, WritableF>::BufferRowBase(Buffer<T>& b, size_t row) :
    BufferView(b), buf(b), dense(vsip::Domain<1>(buf.dc), bufIndex(row)), v(dense), rowBase(row), row(row % buf.dr)
{
    dense.admit(true);
}

template <class T, class VectorT, bool WritableF>
void
BufferRowBase<T, VectorT, WritableF>::setRow(size_t row)
{
    internalFlush(row, true);
}

// switch to the specified row and rebind
template <class T, class VectorT, bool WritableF>
void
BufferRowBase<T, VectorT, WritableF>::internalFlush(size_t row, bool rebind)
{
    rowBase = row;
    dense.release(WritableF);

    if (WritableF && this->row < buf.wr) {
        vsip::Dense<1, T> d(vsip::Domain<1>(buf.dc), bufIndex(this->row + buf.dr));
        vsip::Vector<T> v2(d);
        d.admit(true);
        v2 = v;
        d.release(true);
    }

    if (rebind) {
        row %= buf.dr;
        dense.rebind(bufIndex(row));
        this->row = row;
        dense.admit(true);
    }
}

template <class T, class VectorT, bool WritableF>
BufferRowBase<T, VectorT, WritableF>::~BufferRowBase()
{
    if (WritableF && row < buf.wr) {
        vsip::Dense<1, T> d(vsip::Domain<1>(buf.dc), bufIndex(row + buf.dr));
        vsip::Vector<T> v2(d);
        d.admit(true);
        v2 = v;
        d.release(true);
    }

    dense.release(WritableF);
}

//
//
//
/**
   \brief Bind a vsip::Vector to one row of data within a Buffer.

   If the flush flag is set at object creation, Buffer consistency is maintained by copying the row to the wrapped area
   as necessary whenever
   - a different row is selected
   - flush() is called
   - the object is destroyed

   Whenever read-only access is acceptable, const_BufferRow provides the same features as BufferRow, except for the
   flush().

   BufferRow could not simply subclass vsip::Vector since
   - Vector(length) allocates a new block -- not good for user-allocated memory
   - Vector(Dense&) needs the Dense to be constructed *before* the Vector
   - the overloaded operator= causes problems

   Whenever a row is selected, modular arithmetic is used to ensure that a valid row is selected; this allows BufferRow
   to be used as a rolling buffer...

   \anchor fig1
   \image html BufferRow.png "Placement of a BufferRow within a Buffer"

   Example usage:
   \verbatim
   Buffer<int> b(10, 10, 2, 2);
   Buffer<int>::Row r(b);
   r.v=1; // sets the first row of b to 1
   r.setRow(1); // selects the second row of b
   r.v=2;
   r++; // selects the third row of b
   r.v=3
   r.setRow(33); // 33 mod 10 = 3 -- selects the fourth row of b
   r.v=33;
   r.flush(); // ensures that this change propagates to the wrap area \endverbatim
*/
template <class T>
class BufferRow : public BufferRowBase<T, vsip::Vector<T>, true> {
public:
    BufferRow(Buffer<T>& b, size_t row = 0) : BufferRowBase<T, vsip::Vector<T>, true>(b, row) {}

    /**
       \brief Force the view to be flushed (written to the buffer)

       This function ignores the flush flag set at object creation.  It is useful if you need to ensure consistency of
       the underlying Buffer without changing rows.
    */
    void flush();
};

template <class T>
void
BufferRow<T>::flush()
{
    this->rebind(this->rowBase, false);
}

//
//
//
/**
   \brief Bind a vsip::const_Vector to one row of data within a Buffer.

   \note If possible, use const_BufferRow instead of a normal BufferRow.
*/
template <class T>
class const_BufferRow : public BufferRowBase<T, vsip::const_Vector<T>, false> {
public:
    const_BufferRow(Buffer<T>& b, size_t row = 0) : BufferRowBase<T, vsip::const_Vector<T>, false>(b, row) {}
};

/** \file
 */

#endif
