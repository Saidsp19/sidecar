#ifndef SIDECAR_IO_LINEBUFFER_H // -*- C++ -*-
#define SIDECAR_IO_LINEBUFFER_H

#include <exception>
#include <fstream>
#include <iosfwd>
#include <streambuf>
#include <string>
#include <vector>

#include "boost/scoped_ptr.hpp"

#include "Utils/Exception.h"

namespace SideCar {
namespace IO {

/** Specialization of std::streambuf that reads whole lines of text, counting the number of lines read in.
    Useful to use as the buffer for an input stream so that when an error is encountered during processing, some
    reference where the error occured can be given in an exception.

    Overrides setbuf(), overflow(), and underflow() to maintain the line count.
*/
class LineBuffer : public std::streambuf {
public:
    using BufferType = std::vector<char>;

    /** Constructor.

        \param src streambuf instance to use for raw data

        \param own indicates whether this instance owns the src pointer
    */
    explicit LineBuffer(std::streambuf* src);

    /** Destructor. Zaps the parent buffer pointers to help catch any misuse of a destructed LineBufffer object.
     */
    ~LineBuffer();

    /** Obtain the current line number.

        \return line number
    */
    int getLineNumber() const { return lineNumber_; }

    /** Obtain the contents of the input line buffer.

        \return reference to buffer
    */
    const BufferType& getBuffer() const { return buffer_; };

protected:
    /** Override of streambuf method. Forward call to the active streambuf object.

        \param ptr character buffer to use

        \param len size of buffer

        \return reference to self as a std::streambuf
    */
    virtual std::streambuf* setbuf(char* ptr, std::streamsize len) { return src_->pubsetbuf(ptr, len); }

    /** Override of streambuf method. We don't support `put' operations, so we simply return EOF.

        \return EOF
    */
    int overflow(int) { return traits_type::eof(); }

    /** Override of streambuf method. Keep track of line numbers as characters are added to the internal buffer.

        \return next character found in buffer, or EOF if end-of-file
    */
    int underflow();

private:
    /** Copying is prohibited.
     */
    LineBuffer(const LineBuffer&);

    /** Assigning is prohibited.
     */
    LineBuffer& operator=(const LineBuffer&);

    std::streambuf* src_;
    unsigned int lineNumber_;
    BufferType buffer_;
};

/** Derivation of std::istream that uses a LineBuffer for input buffering. Provides simple error-reporting
    mechanism that throws a LineBufferStream::InputFailure exception, showing an error message and the line in
    the input buffer present when the error was raised.
*/
class LineBufferStream : public std::istream {
public:
    using traits_type = std::istream::traits_type;

    /** Constructor. Creates a LineBuffer object wrapper for the given streambuf object.
     */
    LineBufferStream(std::streambuf* buf) : std::istream(lineBuffer_ = new LineBuffer(buf)) {}

    /** Destructor. Disposes of LineBuffer object.
     */
    virtual ~LineBufferStream() { delete rdbuf(); }

    /** Obtain the current line number of the input buffer.

        \return line number
    */
    int getLineNumber() const { return lineBuffer_->getLineNumber(); }

    /** Obtain the contents of the input line buffer.

        \return copy of buffer
    */
    const LineBuffer::BufferType& getBuffer() const { return lineBuffer_->getBuffer(); }

    /** Write out the line number and buffer contents to the given stream in the format "Line: <LINE>
        '<BUFFER>'"

        \param os stream to write to
    */
    void printContext(std::ostream& os) const;

    /** Write out line information to a C++ output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const { return os << " Line: " << getLineNumber(); }

private:
    LineBuffer* lineBuffer_;
};

/** C++ output stream inserter for LineBufferStream objects. Simply invokes LineBufferStream::print() method.

    \param os stream to write to

    \param lbs object to write

    \return stream written to
*/
inline std::ostream&
operator<<(std::ostream& os, const LineBufferStream& lbs)
{
    return lbs.print(os);
}

/** Helper class for LineBufferFile that allows it to open a C++ file input stream and get its read buffer to
    pass on to LineBufferStream's constructor. Seee LineBufferFile for the rationale for this.
*/
class LineBufferFileHelper {
public:
    /** Construct a new input file stream that uses a LineBuffer for reading in data.

        \param path the path to the file to open

    */
    LineBufferFileHelper(const char* path) : ifs_(std::make_unique<std::ifstream>(path)) {}

    /** Construct a new input file stream that uses a LineBuffer for reading in data.

        \param path the path to the file to open

    */
    LineBufferFileHelper(const std::string& path) : ifs_(std::make_unique<std::ifstream>(path.c_str())) {}

protected:
    /** Obtain the stream buffer being used.

        \return current std::streambuf object
    */
    std::streambuf* getReadBuffer() { return ifs_->rdbuf(); }

private:
    std::unique_ptr<std::ifstream> ifs_;
};

/** Derivation of LineBufferStream that takes its buffer data from a file. Uses LineBufferFileHelper to properly
    initialize the base class LineBufferStream. LineBufferFile cannot do this by itself since the order of
    initialization for an object is to first initialize base classes before the members of the current class.
    Since we can control the ordering of initialization of base classes, we use LineBufferFileHelper to open the
    C++ file input stream, and then use the read buffer from that operation to pass to the constructor for the
    second base class, LineBufferStream.
*/
class LineBufferFile : public LineBufferFileHelper, public LineBufferStream {
public:
    /** Constructor. Attempt to open a ifstream object and use its buffer for the line input buffer.

        \param path location of file to open
    */
    LineBufferFile(const char* path) : LineBufferFileHelper(path), LineBufferStream(getReadBuffer()), path_(path) {}

    /** Constructor for C++ string paths.

        \param path location of file to open
    */
    LineBufferFile(const std::string& path) : LineBufferFileHelper(path), LineBufferStream(getReadBuffer()), path_(path)
    {
    }

    /** Write out line and file information to a C++ output stream.

        \param os stream to write to

        \return stream written to
    */
    std::ostream& print(std::ostream& os) const { return LineBufferStream::print(os) << " File: '" << path_ << "'"; }

private:
    std::string path_; ///< Location of the file we opened.
};

/** C++ output stream inserter for LineBufferFile objects. Simply invokes LineBufferFile::print() method.

    \param os stream to write to

    \param lbs object to write

    \return stream written to
*/
inline std::ostream&
operator<<(std::ostream& os, const LineBufferFile& lbf)
{
    return lbf.print(os);
}

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
