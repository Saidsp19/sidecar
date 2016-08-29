#ifndef BUFFER_MAT_H
#define BUFFER_MAT_H

#include <vsip/matrix.hpp>
#include <vsip/math.hpp>

/**
   \brief View a submatrix within a Buffer.

   Bind a vsip::Matrix to a (sub)set of the buffer
   Copies changes to the wrapped area if the location changes or BufferMat is destroyed.
   Cannot simply subclass vsip::Matrix since
   - Matrix(rows,cols) allocates a new block -- not good for user-allocated memory
   - Matrix(Dense&) needs the Dense to be constructed *before* the Matrix, but the Dense should be created and destroyed by the BufferMat.
   - the overloaded operator= causes problems

   As illustrated in \ref fig1 "Figure 1", the BufferMat provides a subview of a parent Buffer object.  Unlike the strict alignment of BufferRow and BufferCol within the parent Buffer, BufferMat is somewhat free to move around.  The only requirements are:
   - the maximum size of (rows, cols) be able to fit within the parent Buffer's data dimensions of (Buffer::getDataRows(), Buffer::getDataCols())
   - the first column of the BufferMat must not start before the allocated window column width (-b.wc)
   - the first column of the BufferMat must not start past the last column of the data area

   Acceptable locations for the BufferMat are indicated by shading in \ref fig1 "Figure 1".

   As with the BufferRow, the row indices of BufferMat are taken modulo b.dr to allow for a rolling window into the data.

   \anchor fig1
   \image html BufferMat.png "Placement of a BufferMat within a Buffer"

   Only matrices up to size (Buffer::getWindowRows(), Buffer::getWindowRows()) are guaranteed to be within the Buffer; lager matrices may fall outside the allocated space depending on their initial row and column within the parent Buffer.
*/
template<class T>
class BufferMat : private BufferView
{
public:
    using SubMat = vsip::Matrix<T, vsip::impl::Subset_block<vsip::Dense<2, T> > >;

    /**
       \brief View a subset of the Buffer as a vsip::Matrix

       Creates a Matrix of size (rows, cols) with the top-left corner located at (row, col) in the parent Buffer.

       If rows=0, then use all b.dr rows.  Similarly, cols=0 selects all b.dc columns.

       \param b Buffer to view

       \param rows Number of rows to view.  Limited to rows <= Buffer::getDataRows()

       \param cols Number of columns to view.  Limited to cols <= Buffer::getDataCols()

       \param row Index of the first row in the parent Buffer

       \param col Index of the first column in the parent Buffer.   Limited to the range - Buffer::getWindowCols() < col < Buffer::getDataCols()

       \param flush Whether this view should change the underlying buffer
    */
    BufferMat(Buffer<T> &b, size_t rows=0, size_t cols=0, size_t row=0, int col=0, bool flush=true);
    ~BufferMat();
    /**
       \brief Sets the location of the first row and column in the parent Buffer.

       \param row Index of the first row in the parent Buffer

       \param col Index of the first column in the parent Buffer.   Limited to the range - Buffer::getWindowCols() < col < Buffer::getDataCols()
    */
    void setStart(size_t row=0, int col=0);
    /**
       \brief Sets the location of the first row in the parent Buffer.

       \param row Index of the first row in the parent Buffer
    */
    inline void setRow(size_t row=0) { setStart(row, this->col); }
    /**
       \brief Sets the location of the first column in the parent Buffer.

       \param col Index of the first column in the parent Buffer.  Limited to the range - Buffer::getWindowCols() < col < Buffer::getDataCols()
    */
    inline void setCol(int col=0) { setStart(this->row, col); }
    /**

       \return Index of the row which is being viewed
    */
    inline size_t getRow() const { return row; }
    /**

       \return Index of the column which is being viewed
    */
    inline int getCol() const { return col; }

    /**
       \brief Force the view to be flushed (written to the buffer)

       This function ignores the flush flag set at object creation.  It is useful if you need to ensure consistency of the underlying Buffer without changing the row or column.
    */
    inline void flush() { return internalflush(true); }

private:
    const Buffer<T>& buf;
    size_t rows, cols; // size of the matrix
    size_t row, col; // location of the top-left corner
    vsip::Dense<2, T> dense;
    vsip::Matrix<T> allmat;
    vsip::impl::Subset_block<vsip::Dense<2, T> > subview;
public:
    /**
       \brief The vsip::Matrix which is mapped to this subview

       This member is made public for ease of use; any operations on it will use the vsip::Matrix operator overloads.
    */
    SubMat m;
private:
    bool doflush; // indicates whether to flush the matrix (to the wrap rows)

    inline T *bufIndex(size_t r, int c)
    {
        buf.wc!=0 ? assert(c > -(int)buf.wc) : assert(c>=0);
        assert(c < (int)buf.dc);

        int index=(r%buf.dr)*buf.cols + c;
        if(index<0)
            index += buf.dr*buf.cols;
        return buf.buf + index;
    }

    void internalflush(bool rebind);

private:
    // BufferView interface
    //
    void pause()
    {
        internalflush(false);
    }
    void resume()
    {
        dense.rebind(bufIndex(row, col));
        dense.admit(true);
    }
};

template<class T>
BufferMat<T>::BufferMat(Buffer<T> &b, size_t rows, size_t cols, size_t row, int col, bool flush)
: BufferView(b),
    buf(b),
    rows(rows==0 ? buf.dr : rows),
    cols(cols==0 ? buf.dc : cols),
    row(row),
    col(col),
    dense(vsip::Domain<2>(buf.rows, buf.cols), bufIndex(this->row, this->col)),
    allmat(dense),
    subview(vsip::Domain<2>(this->rows, this->cols), allmat.block()),
    m(subview),
    doflush(flush)
{
    assert(this->rows <= buf.dr);
    assert(this->cols <= buf.dc);

    // for now, always try to load
    dense.admit(true);
}

template<class T>
void BufferMat<T>::setStart(size_t row, int col)
{
    if(doflush)
    {
        internalflush(false);
    }
    else
    {
        dense.release(false);
    }

    dense.rebind(bufIndex(row, col));
    dense.admit(true);
    this->row=row;
    this->col=col;
}

template<class T>
void BufferMat<T>::internalflush(bool rebind)
{
    // Copy the top few rows to the wrap zone
    if(row<buf.wr)
    {
        // Force a flush of the matrix
        dense.release(true);
        dense.admit(true);

        // Perform the copy
        size_t overlap=buf.wr-row;

        vsip::Dense<2, T> origin(vsip::Domain<2>(buf.rows, buf.cols), bufIndex(0, 0));
        vsip::Matrix<T> normalview(origin);
        origin.admit(true);
        vsip::Domain<2> d(vsip::Domain<1>(buf.dr+row, 1, overlap), vsip::Domain<1>(col, 1, cols));
        normalview(d)=m(vsip::Domain<2>(vsip::Domain<1>(0, 1, overlap), cols));
        origin.release(true);
    }
    // Copy the wrap zone back to the top
    if(row+rows>buf.dr)
    {
        // Force a flush of the matrix
        dense.release(true);
        dense.admit(true);

        // Perform the copy
        size_t overlap=row+rows-buf.dr;

        vsip::Dense<2, T> origin(vsip::Domain<2>(buf.rows, buf.cols), bufIndex(0, 0));
        vsip::Matrix<T> normalview(origin);
        origin.admit(true);
        vsip::Domain<2> d(vsip::Domain<1>(0, 1, overlap), vsip::Domain<1>(col, 1, cols));
        normalview(d)=m(vsip::Domain<2>(vsip::Domain<1>(buf.dr-row, 1, overlap), cols));
        origin.release(true);
    }

    dense.release(true);

    if(rebind)
        dense.admit(true);
}

template<class T>
BufferMat<T>::~BufferMat()
{
    if(doflush)
    {
        internalflush(false);
    }
    else
    {
        dense.release(false);
    }
}

/** \file
 */

#endif
