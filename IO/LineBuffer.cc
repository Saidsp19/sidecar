#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>

#include "LineBuffer.h"

using namespace SideCar::IO;

LineBuffer::LineBuffer(std::streambuf* src) : src_(src), lineNumber_(0), buffer_()
{
    buffer_.reserve(1024);
}

LineBuffer::~LineBuffer()
{
    setg(NULL, NULL, NULL);
}

int
LineBuffer::underflow()
{
    if (!src_) return traits_type::eof();

    // See if we have anything buffered to give.
    //
    if (gptr() < egptr()) return *gptr();

    // Read a whole line from our source.
    //
    buffer_.clear();
    bool inComment = false;
    bool done = false;
    do {
        // Get next character from the original stream buffer.
        //
        int c = src_->sbumpc();
        switch (c) {
        case '#': // Comment character
            inComment = true;
            break;

        case ' ':
        case '\t':
            if (!buffer_.empty() && !inComment) buffer_.push_back(c);
            break;

        case '\n': // End of line?
            ++lineNumber_;
            inComment = false;
            if (!buffer_.empty()) {
                buffer_.push_back(c);
                done = true;
            }
            break;

        default: // Check for EOF
            if (c == traits_type::eof()) {
                done = true;
            } else if (!inComment) {
                buffer_.push_back(c);
            }
            break;
        }
    } while (!done);

    if (buffer_.empty()) {
        setg(NULL, NULL, NULL);
        return traits_type::eof();
    }

    setg(&(buffer_[0]), &(buffer_[0]), &(buffer_[buffer_.size()]));
    return buffer_[0];
}

void
LineBufferStream::printContext(std::ostream& os) const
{
    os << "Line: " << lineBuffer_->getLineNumber() << " '";
    const LineBuffer::BufferType& buf = lineBuffer_->getBuffer();

    // Copy over the contents of the line buffer, but do not copy the tail end-of-line character if present.
    //
    if (!buf.empty()) {
        LineBuffer::BufferType::const_iterator z = buf.end();
        if (buf.back() == '\n') --z;
        std::copy(buf.begin(), z, std::ostream_iterator<char>(os));
    }
    os << "'";
}
