#ifndef SIDECAR_IO_GATHERWRITER_H // -*- C++ -*-
#define SIDECAR_IO_GATHERWRITER_H

#include "IO/Writers.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

/** Collector of CDR-encoded SideCar messages for writing to a IO::Writer device once some limit has been
    reached. Contains two parameters that control how many messages it collects before ending to the IO::Writer
    device:

    - sizeLimit -- number of bytes to collect before sending
    - countLimit -- number of messages to collect before sending

    If neither limit is set, or both are set to zero, then the collector will simply write each message as it
    receives them.
*/
class GatherWriter {
public:
    /** Log device to use for GatherWriter object messages. Has the name "SideCar.IO.GatherWriter.

        \return log device to use
    */
    static Logger::Log& Log();

    /** Default constructor. Initializes limits to zero, disabling caching.

        \param writer device to use to write out message data
    */
    GatherWriter(Writer& writer);

    /** Constructor. Initializes limits to given values.

        \param writer device to use to write out message data

        \param sizeLimit number of bytes to collect before sending

        \param countLimit number of messages to collect before sending
    */
    GatherWriter(Writer& writer, size_t sizeLimit, size_t countLimit);

    /** Destructor. Flushes any held data.
     */
    ~GatherWriter();

    /** Obtain the current message size limit.

        \return size limit
    */
    size_t getSizeLimit() const { return sizeLimit_; }

    /** Change the message size limit.

        \param sizeLimit new size limit
    */
    void setSizeLimit(size_t sizeLimit);

    /** Obtain the current message count limit

        \return count limit
    */
    size_t getCountLimit() const { return countLimit_; }

    /** Change the message count limit.

        \param countLimit new count limit
    */
    void setCountLimit(size_t countLimit);

    /** Add encoded message data to the cache.

        \param data encoded message data

        \return true if no error
    */
    bool add(ACE_Message_Block* data);

    /** Force the write of any cached encoded message data.
     */
    void flush();

    /** Determine if all writes have succeeded so far.

        \return true if so.
    */
    bool isOK() const { return ok_; }

    /** Obtain the current accumulated message size.

        \return held message byte count
    */
    size_t getSize() const { return size_; }

    /** Obtain the current accumulated message count.

        \return held message count
    */
    size_t getCount() const { return count_; }

private:
    /** Determine if the limits have been met/passed.

        \return
    */
    bool needFlush() const;

    Writer& writer_;           ///< Writer device that does the writing
    size_t sizeLimit_;         ///< Amount to gather in bytes
    size_t countLimit_;        ///< Amount to gather in message count
    ACE_Message_Block* first_; ///< First message to write
    ACE_Message_Block* last_;  ///< Last message to write
    size_t size_;              ///< Current number of bytes gathered
    size_t count_;             ///< Current number of messages gathered
    bool ok_;                  ///< Set to false on first failure to write
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
