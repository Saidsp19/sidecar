#ifndef BUFFER_COL_H
#define BUFFER_COL_H

#include <vsip/vector.hpp>
#include <vsip/impl/subblock.hpp>
#include <vsip/math.hpp>
#include "ModelView.h"

/**
   \brief Bind a vsip::Vector to one column of data within a Buffer.

   Copies changes to the wrapped area if the column changes or BufferCol is destroyed. Cannot simply subclass
   vsip::Vector since
   
   - Vector(length) allocates a new block -- not good for user-allocated memory
   - Vector(Dense&) needs the Dense to be constructed *before* the Vector but the Dense should be created and
     destroyed by the BufferCol. 
   - the overloaded operator= causes problems

   \anchor fig1
   \image html BufferCol.png "Placement of a BufferCol within a Buffer"
*/
template<class T>
class BufferCol : private BufferView
{
public:
    using ColVector = vsip::Vector<T, vsip::impl::Subset_block<vsip::Dense<1, T> > >;

    BufferCol(const Buffer<T> &b, size_t col=0, bool flush=true);
    ~BufferCol();

    /**
       \brief Change which column to view

       \param Index of the new column to view
    */
    void setCol(size_t col=0);
    /**

       \return Index of the column which is being viewed
    */
    size_t getCol() { return col; }

    /**
       \brief Force the view to be flushed (written to the buffer)

       This function ignores the flush flag set at object creation.  It is useful if you need to ensure consistency of the underlying Buffer without changing columns.
    */
    void flush();

private:
    const Buffer<T>& buf;
    vsip::Dense<1, T> dense;
    vsip::Vector<T> allvec;
    vsip::impl::Subset_block<vsip::Dense<1, T> > subview;
public:
    /**
       \brief The vsip::Vector which is mapped to this column

       This member is made public for ease of use; any operations on it will use the vsip::Vector operator overloads.
    */
    ColVector v;
private:
    inline T *bufIndex(size_t c) { return buf.buf + c; }
    size_t col; // which col we are revealing
    bool doflush; // indicates whether to flush the column (to the wrap rows)

private:
    // BufferView interface
    //
    void pause() { this->dense.release(doflush); }
    void resume()
    {
        dense.rebind(bufIndex(col));
        dense.admit(true);
    }
};

template<class T>
BufferCol<T>::BufferCol(const Buffer<T> &b, size_t col, bool flush)
: buf(b),
    dense(vsip::Domain<1>(buf.length), bufIndex(0)),
    allvec(dense),
    subview(vsip::Domain<1>(col, buf.cols, buf.dr), allvec.block()),
    v(subview),
    col(col),
    doflush(flush)
{
    assert(col<buf.dc);
    dense.admit(true);
}

template<class T>
void BufferCol<T>::setCol(size_t col)
{
    assert(col<buf.dc);

    // Copy the top few rows to the wrap zone
    if(doflush && buf.wr>0)
    {
        vsip::Domain<1> d(buf.dr*buf.cols, buf.cols, buf.wr);
        allvec(d)=v(vsip::Domain<1>(0, 1, buf.wr));
    }

    dense.release(doflush);

    col%=buf.dc;

    dense.rebind(bufIndex(col));
    dense.admit(true);
    this->col=col;
}

template<class T>
void BufferCol<T>::flush()
{
    // Copy the top few rows to the wrap zone
    if(buf.wr>0)
    {
        vsip::Domain<1> d(buf.dr*buf.cols, buf.cols, buf.wr);
        allvec(d)=v(vsip::Domain<1>(0, 1, buf.wr));
    }

    dense.release(true);
    dense.admit(true);
}

template<class T>
BufferCol<T>::~BufferCol()
{
    // Copy the top few rows to the wrap zone
    if(doflush && buf.wr>0)
    {
        vsip::Domain<1> d(buf.dr*buf.cols, buf.cols, buf.wr);
        allvec(d)=v(vsip::Domain<1>(0, 1, buf.wr));
    }

    dense.release(doflush);
}

/** \file
 */

#endif
