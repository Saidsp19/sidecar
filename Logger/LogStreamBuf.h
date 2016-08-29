#ifndef LOGGER_STREAM_H		// -*- C++ -*-
#define LOGGER_STREAM_H

#include <sstream>
#include "Logger/Priority.h"

namespace Logger {

class Log;

/** Enhanced std::stringbuf that forwards `finished' log messages to a Log device. The sync() method handles
    this message forwarding. It overrides the std::stringbuf method, and is invoked when the owning stream
    wishes to flush the output.
*/
class LogStreamBuf : public std::stringbuf
{
public:

    /** Constructor.
     */
    LogStreamBuf()
	: std::stringbuf(std::ios_base::out), log_(0), level_() {}

    /** Destuctor. Flush any unfinished message.
     */
    virtual ~LogStreamBuf();

    /** Assign a Log device and message priority level for the next message accumulated by the buffer.

        \param log Log device that will receive the finished message text

        \param level priority level for the message text
    */
    void setInfo(Log* log, Priority::Level level);

private:

    /** Override of std::stringbuf method. Sends contents of string buffer to associated Log device.

        \return value of std::stringbuf::sync() call
    */
    virtual int sync();

    Log* log_;			///< Log device to receive message texts
    Priority::Level level_;	///< Priority level for message texts
};

/** Enhanced std::stringbuf that ignores all log messages. Used for the null log stream which does not write any
    data.
*/
class NullLogStreamBuf : public std::stringbuf
{
public:

    /** Constructor.
     */
    NullLogStreamBuf() : std::stringbuf(std::ios_base::out) {}

private:

    /** Override of stringbuf method. Don't do any writes.

	\param s pointer to characters to write

	\param n number of characters to write

	\return character count
    */
    virtual std::streamsize xsputn(const char_type* s, std::streamsize n)
	{ return n; }
};

} // namespace Logger

/** \file
 */

#endif
