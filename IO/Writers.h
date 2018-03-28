#ifndef SIDECAR_IO_WRITERS_H // -*- C++ -*-
#define SIDECAR_IO_WRITERS_H

#include <sys/uio.h> // for struct iovec
#include <vector>

#include "ace/CDR_Stream.h"
#include "ace/FILE_IO.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/SOCK_Stream.h"

#include "boost/shared_ptr.hpp"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

class MessageManager;
class Task;

/** Collection of device-specific classes that do the work of sending raw data. Each device shall provide a
    writeToDevice() method that takes a pointer to an iovec structure and a count of how many iovec elements
    there are. The routine must return the number of bytes successfully written (which may be less than the sum
    indicated by the iovec entries), or -1 if it encountered an error while writing.

    All of the classes here use ACE classes to do all of the hard work connecting and managing connections.
*/
namespace WriterDevices {

/** Device that writes to a file.
 */
class File {
public:
    /** Constructor for new device.
     */
    File() : device_() {}

    /** Obtain reference to ACE file device.

        \return file device
    */
    ACE_FILE_IO& getDevice() { return device_; }

    /** Obtain reference to ACE file device.

        \return file device
    */
    const ACE_FILE_IO& getDevice() const { return device_; }

protected:
    /** Send data to the device.

        \param iov address of first iovec structure to use

        \param count number of iovec structures to process

        \return number of bytes successfully written to device, or -1 if error
    */
    ssize_t writeToDevice(const iovec* iov, int count) { return device_.send(iov, count); }

private:
    ACE_FILE_IO device_; ///< File device for data writes
};

/** Device that writes data to a network socket using TCP.
 */
class TCPSocket {
public:
    /** Constructor for new device.
     */
    TCPSocket() : device_() {}

    /** Obtain reference to ACE socket device.

        \return socket device
    */
    ACE_SOCK_Stream& getDevice() { return device_; }

    /** Obtain reference to ACE socket device.

        \return socket device
    */
    const ACE_SOCK_Stream& getDevice() const { return device_; }

protected:
    /** Send data to the device.

        \param iov address of first iovec structure to use

        \param count number of iovec structures to process

        \return number of bytes successfully written to device, or -1 if error
    */
    ssize_t writeToDevice(const iovec* iov, int count) { return device_.sendv(iov, count); }

private:
    ACE_SOCK_Stream device_; ///< Socket device for data fetches
};

/** Device that writes to a multicast socket.
 */
class MulticastSocket {
public:
    static Logger::Log& Log();

    /** Constructor for new reader.
     */
    MulticastSocket();

    /** Obtain reference to socket device.

        \return socket device
    */
    ACE_SOCK_Dgram_Mcast& getDevice() { return device_; }

    /** Obtain read-only reference to socket device.

        \return socket device
    */
    const ACE_SOCK_Dgram_Mcast& getDevice() const { return device_; }

protected:
    /** Send data to the device.

        \param iov address of first iovec structure to use

        \param count number of iovec structures to process

        \return number of bytes successfully written to device, or -1 if error
    */
    ssize_t writeToDevice(const iovec* iov, int count);

private:
    ACE_SOCK_Dgram_Mcast device_;
};

/** Device that writes to a unicast socket.
 */
class UDPSocket {
public:
    static Logger::Log& Log();

    /** Constructor for new reader.
     */
    UDPSocket();

    /** Obtain reference to socket device.

        \return socket device
    */
    ACE_SOCK_Dgram& getDevice() { return device_; }

    /** Obtain read-only reference to socket device.

        \return socket device
    */
    const ACE_SOCK_Dgram& getDevice() const { return device_; }

    /** Set the address of the remote host to write to

        \param remoteAddress IP address of remote host
    */
    void setRemoteAddress(const ACE_INET_Addr& remoteAddress) { remoteAddress_ = remoteAddress; }

protected:
    /** Send data to the device.

        \param iov address of first iovec structure to use

        \param count number of iovec structures to process

        \return number of bytes successfully written to device, or -1 if error
    */
    ssize_t writeToDevice(const iovec* iov, int count);

private:
    ACE_SOCK_Dgram device_;
    ACE_INET_Addr remoteAddress_;
};

} // end namespace WriterDevices

/** C++ interface for an ::iovec struct.
 */
struct IOV : public iovec {
    /** Constructor. Initializes to null values.
     */
    IOV()
    {
        iov_base = 0;
        iov_len = 0;
    }

    /** Constructor. Initializes ::iovec struct with data pointer and byte count.

        \param addr location of first byte to write

        \param size number of bytes to write
    */
    IOV(const void* addr, size_t size)
    {
        iov_base = const_cast<void*>(addr);
        iov_len = size;
    }
};

/** Vector of IOV entries. Used in gather-write methods such as writev.
 */
class IOVVector : public std::vector<IOV> {
public:
    using Super = std::vector<IOV>;

    /** Create IOV objects from the contents of an ACE_Message_Block and add to the collection.

        \param data block to add
    */
    size_t push_back(const ACE_Message_Block* data);
};

/** Abstract base class for data writers. A writer simply submits a block of data to a device object, such as a
    file or socket. The writer itself has no idea how data is transported; it only knows how to get the data to
    the device.
*/
class Writer {
public:
    using Ref = boost::shared_ptr<Writer>;

    /** Obtain the log device to use for log messages.

        \return
    */
    static Logger::Log& Log();

    /** Constructor.
     */
    Writer() : closing_(0), lastError_(0) {}

    /** Destructor.
     */
    virtual ~Writer() {}

    /** Set a flag to tell the writer that the device is being closed. This will break the writer out of the
        while loop in writeEncoded if it is encountering EAGAIN errors while writing data.
    */
    void setClosingSignal() { closing_ = 1; }

    /** Reset the flag that tells the writer that the device is being closed.
     */
    void resetClosingSignal() { closing_ = 0; }

    /** Determine if the writer has been commanded to shut down.

        \return true if so
    */
    bool isClosing() const { return closing_; }

    /** Write out an already-encoded data block.

        \param data block to write

        \return true if successful, false otherwise
    */
    bool writeEncoded(size_t messageCount, ACE_Message_Block* data);

    /** Write out the message managed by a MessageManager object. Invokes MessageManager::getEncoded() to obtain
        the underlying ACE_Message_Block data block.

        \param mm container with the data to write

        \return true if successful, false otherwise
    */
    bool write(const MessageManager& mm);

    int getLastError() const { return lastError_; }

protected:
    /** Prototype of method that writes data to a device. The data to write exists as an array of pointer/length
        pairs.

        \param iov pointer to first iov to send

        \param count number of iov entries to send

        \return number of bytes sent if >= 0; error condition if < 0
    */
    virtual ssize_t writeToDevice(const iovec* iov, int count) = 0;

private:
    volatile int closing_;
    int lastError_;
};

/** Abstract base class for a data writer. The template argument _D is a device writer class that provides a
    writeToDevice(const ACE_OutputCDR&) method.
*/
template <typename _D>
class TWriter : public Writer, public _D {
public:
    using Ref = boost::shared_ptr<TWriter<_D>>;

    /** Factory method that creates a new TWriter<_D> object.

        \return new TWriter<_D> object
    */
    static Ref Make()
    {
        Ref ref(new TWriter<_D>);
        return ref;
    }

    /** Type of the device that writes out raw data for this writer. Device must provide a writeToDevice()
        method.
    */
    using DeviceType = _D;

    /** Constructor.

        \param blockSize size of initial buffer
    */
    TWriter() : Writer(), DeviceType() {}

    int close() { return DeviceType::getDevice().close(); }

protected:
    /** Implementation of MessageWriterBase prototype. Forwards the writing request to the base device class.
        Utilizes the gather-write mechanism of the underlying device.

        \param iov first iovec to write

        \param count number of iovec entries to write.

        \return number of bytes written if >= 0, and error condition if < 0.
    */
    ssize_t writeToDevice(const iovec* iov, int count) { return _D::writeToDevice(iov, count); }
};

/** Writer that sends raw data to a file device.
 */
class FileWriter : public TWriter<WriterDevices::File> {
public:
    /** Class type synonym for our parent class
     */
    using Super = TWriter<WriterDevices::File>;

    /** Constructor for new writer.
     */
    FileWriter() : Super() {}
};

/** Writer that sends raw data to a TCP socket device.
 */
class TCPSocketWriter : public TWriter<WriterDevices::TCPSocket> {
public:
    /** Class type synonym for our parent class
     */
    using Super = TWriter<WriterDevices::TCPSocket>;

    /** Constructor for new writer.
     */
    TCPSocketWriter() : Super() {}
};

/** Writer that sends raw data to a UDP socket device.
 */
class UDPSocketWriter : public TWriter<WriterDevices::UDPSocket> {
public:
    /** Class type synonym for our parent class
     */
    using Super = TWriter<WriterDevices::UDPSocket>;

    /** Constructor for new writer.
     */
    UDPSocketWriter() : Super() {}
};

/** Writer that sends raw data to a multicast UDP socket device.
 */
class MulticastSocketWriter : public TWriter<WriterDevices::MulticastSocket> {
public:
    /** Class type synonym for our parent class
     */
    using Super = TWriter<WriterDevices::MulticastSocket>;

    /** Constructor for new writer.
     */
    MulticastSocketWriter() : Super() {}
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
