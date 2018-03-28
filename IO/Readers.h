#ifndef SIDECAR_IO_READERS_H // -*- C++ -*-
#define SIDECAR_IO_READERS_H

#include "ace/CDR_Stream.h"
#include "ace/FILE_IO.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/SOCK_Stream.h"
#include "ace/Task_T.h"

#include "boost/shared_ptr.hpp"

#include "IO/Preamble.h"
#include "IO/Stats.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace IO {

class Task;

/** Collection of device-specific classes that do the work of reading raw data.
 */
namespace ReaderDevices {

/** Device that gets data from a file
 */
class File {
public:
    /** Constructor for new reader.
     */
    File() : device_() {}

    /** Obtain reference to file device.

        \return file device
    */
    ACE_FILE_IO& getDevice() { return device_; }

    /** Obtain read-only reference to file device.

        \return file device
    */
    const ACE_FILE_IO& getDevice() const { return device_; }

protected:
    /** Obtain data from the device.

        \param addr where to place fetched data

        \param size how much data to fetch

        \return number of bytes fetched if > 0; EOF if == 0; and error condition if < 0
    */
    ssize_t fetchFromDevice(void* addr, size_t size) { return device_.recv_n(addr, size); }

private:
    ACE_FILE_IO device_; ///< File device for data fetches
};

/** Device that gets data from a network socket using TCP.
 */
class TCPSocket {
public:
    /** Constructor for new reader.
     */
    TCPSocket() : device_() {}

    /** Obtain reference to socket device.

        \return socket device
    */
    ACE_SOCK_Stream& getDevice() { return device_; }

    /** Obtain read-only reference to socket device.

        \return socket device
    */
    const ACE_SOCK_Stream& getDevice() const { return device_; }

protected:
    /** Obtain data from the device.

        \param addr where to place fetched data

        \param size how much data to fetch

        \return number of bytes fetched if > 0; EOF if == 0; and error
        condition if < 0
    */
    ssize_t fetchFromDevice(void* addr, size_t size) { return device_.recv(addr, size); }

private:
    ACE_SOCK_Stream device_; ///< Socket device for data fetches
};

/** Device that gets data from a network socket set up for UDP messages.
 */
class UDPSocket {
public:
    /** Constructor for new reader.
     */
    UDPSocket() : device_(), timeout_(0) {}

    /** Obtain reference to socket device.

        \return socket device
    */
    ACE_SOCK_Dgram& getDevice() { return device_; }

    /** Obtain read-only reference to socket device.

        \return socket device
    */
    const ACE_SOCK_Dgram& getDevice() const { return device_; }

    /** Obtain the address/port of the host that sent the last UDP message we received in fetchFromDevice().

        \return ACE_INET_Addr reference
    */
    const ACE_INET_Addr& getRemoteAddress() const { return remoteAddress_; }

    /** Set a timeout value for fetches from a UDP socket. Useful when fetchFromDevice() runs a loop in a
        thread, causing recv() to block for a set amount of time before returning with a -1.

        \param timeout the amount of time to wait for data
    */
    void setFetchTimeout(const ACE_Time_Value* timeout) { timeout_ = timeout; }

    bool open(const ACE_Addr& address) { return device_.open(address) != -1; }

protected:
    /** Obtain data from the device.

        \param addr where to place fetched data

        \param size how much data to fetch

        \return number of bytes fetched if > 0; EOF if == 0; and error condition if < 0
    */
    ssize_t fetchFromDevice(void* addr, size_t size) { return device_.recv(addr, size, remoteAddress_, 0, timeout_); }

private:
    ACE_SOCK_Dgram device_; ///< Socket device for data fetches
    ACE_INET_Addr remoteAddress_;
    const ACE_Time_Value* timeout_;
};

/** Device that gets data from a network socket set up for multicast UDP messages.
 */
class MulticastSocket {
public:
    static Logger::Log& Log();

    /** Constructor for new reader.
     */
    MulticastSocket() : device_(), timeout_(0) {}

    /** Obtain reference to socket device.

        \return socket device
    */
    ACE_SOCK_Dgram_Mcast& getDevice() { return device_; }

    /** Obtain read-only reference to socket device.

        \return socket device
    */
    const ACE_SOCK_Dgram_Mcast& getDevice() const { return device_; }

    /** Obtain the address/port of the host that sent the last UDP message we received in fetchFromDevice().

        \return ACE_INET_Addr reference
    */
    const ACE_INET_Addr& getRemoteAddress() const { return remoteAddress_; }

    /** Set a timeout value for fetches from a UDP socket. Useful when fetchFromDevice() runs a loop in a
        thread, causing recv() to block for a set amount of time before returning with a -1.

        \param timeout the amount of time to wait for data
    */
    void setFetchTimeout(const ACE_Time_Value* timeout) { timeout_ = timeout; }

    /** Establish a connection to a remote host at a given address

        \param address remote host to communicate with

        \return true if successful
    */
    bool join(const ACE_INET_Addr& address);

protected:
    /** Obtain data from the device.

        \param addr where to place fetched data

        \param size how much data to fetch

        \return number of bytes fetched if > 0; EOF if == 0; and error condition if < 0
    */
    ssize_t fetchFromDevice(void* addr, size_t size) { return device_.recv(addr, size, remoteAddress_, 0, timeout_); }

private:
    ACE_SOCK_Dgram_Mcast device_; ///< Socket device for data fetches
    ACE_INET_Addr remoteAddress_;
    const ACE_Time_Value* timeout_;
};

} // end namespace ReaderDevices

/** Abstract base class for readers. A reader accumulates data from a device until it has all of the data of a
    complete message. All SideCar messages have a header (see SideCar::Messages::Header class) which contains a
    size in bytes. A reader simply adds incoming bytes to an ACE_Message_Block until the
    ACE_Message_Block::length() == Header::getSize().
*/
class Reader {
public:
    using Ref = boost::shared_ptr<Reader>;

    /** Obtain the log device to use for log messages.

        \return
    */
    static Logger::Log& Log();

    /** Constructor for new reader.
     */
    Reader() : available_(0) {}

    /** Destructor.
     */
    virtual ~Reader();

    /** Read in data from the device and append to the existing ACE_Message_Block.

        \return true if device is still valid, false otherwise
    */
    virtual bool fetchInput() = 0;

    /** Determine if a complete message is available for processing.

        \return true if so
    */
    bool isMessageAvailable() const { return available_; }

    /** Obtain the raw data block available from the reader. This is a one-shot call: future calls will return
        NULL until another message becomes available via the setAvailable() method. NOTE: caller is responsible
        for releasing the block when it is done with it by calling ACE_Message_Block::release() (and not by
        invoking delete on the returned object)

        \return raw data block or NULL if isMessageAvailable() returns false.
    */
    ACE_Message_Block* getMessage();

protected:
    /** Make a complete message available for consumption.

        \param available
    */
    void setAvailable(ACE_Message_Block* available);

private:
    ACE_Message_Block* available_;
};

/** Abstract base class for readers. A reader accumulates data from a device until it has all of the data of a
    complete message. All SideCar messages have a header (see SideCar::Messages::Header class) which contains a
    size in bytes. A reader simply adds incoming bytes to an ACE_Message_Block until the
    ACE_Message_Block::length() == Header::getSize().
*/
class StreamReader : public Reader {
public:
    /** Obtain the log device to use for log messages.

        \return
    */
    static Logger::Log& Log();

    /** Constructor for new reader.

        \param bufferSize starting size for read buffer
    */
    StreamReader(size_t bufferSize);

    /** Destructor.
     */
    ~StreamReader();

    /** Read in data from the device and append to the existing ACE_Message_Block.

        \return true if device is still valid, false otherwise
    */
    bool fetchInput();

    /** Reset the stream by discarding any partially-read messages.
     */
    void reset();

protected:
    /** Prototype of method that fetches data from a device and places it into a specific location.

        \param addr where to place fetched data

        \param size how much data to fetch

        \return number of bytes fetched if > 0; EOF if == 0; and error condition if < 0
    */
    virtual ssize_t fetchFromDevice(void* addr, size_t size) = 0;

private:
    /** Create a new buffer to hold the next message begin built.

        \param bufferSize the initial size of the new buffer
    */
    void makeIncomingBuffer(size_t bufferSize);

    ACE_Message_Block* building_; ///< Message being built
    size_t needed_;               ///< Number of bytes in the message body
    size_t remaining_;            ///< Number of bytes remaining to fetch
    bool needSynch_;
};

/** Abstract base class for readers. A reader accumulates data from a device until it has all of the data of a
    complete message. All SideCar messages have a header (see SideCar::Messages::Header class) which contains a
    size in bytes. A reader simply adds incoming bytes to an ACE_Message_Block until the
    ACE_Message_Block::length() == Header::getSize().
*/
class DatagramReader : public Reader {
public:
    using Ref = boost::shared_ptr<DatagramReader>;

    /** Obtain the log device to use for log messages.

        \return
    */
    static Logger::Log& Log();

    /** Constructor for new reader.

        \param bufferSize starting size for read buffer
    */
    DatagramReader(size_t maxSize);

    /** Destructor.
     */
    ~DatagramReader();

    /** Read in data from the device and append to the existing ACE_Message_Block.

        \return true if device is still valid, false otherwise
    */
    bool fetchInput();

    /** Reset the stream by discarding any partially-read messages. This is a NOP for this class since datagrams
        are always read whole.
    */
    void reset() {}

protected:
    /** Prototype of method that fetches data from a device and places it into a specific location.

        \param addr where to place fetched data

        \param size how much data to fetch

        \return number of bytes fetched if > 0; EOF if == 0; and error
        condition if < 0
    */
    virtual ssize_t fetchFromDevice(void* addr, size_t size) = 0;

    /** Create a new buffer to hold the next message begin built.

        \param bufferSize the initial size of the new buffer
    */
    void makeIncomingBuffer(size_t size);

    ACE_Message_Block* building_; ///< Message being built
};

/** Template class for device-specific readers. The template argument _D is a device to use to actually read in
    the message data.
*/
template <typename _R, typename _D>
class TReader : public _R, public _D {
public:
    using Ref = boost::shared_ptr<TReader<_R, _D>>;

    /** Type of the device providing raw data for this reader. Device must provide a fetchFromDevice() method.
     */
    using ReaderType = _R;
    using DeviceType = _D;

    /** Constructor.

        \param bufferSize starting size for internal read buffer
    */
    TReader(size_t bufferSize) : ReaderType(bufferSize), DeviceType() {}

    int close() { return DeviceType::getDevice().close(); }

protected:
    /** Implementation of the MessageReader prototype. Forwards the fetch request to the base device class.

        \param addr where to place fetched data

        \param size how much data to fetch

        \return number of bytes fetched if > 0; EOF if == 0; and error condition if < 0
    */
    ssize_t fetchFromDevice(void* addr, size_t size) { return DeviceType::fetchFromDevice(addr, size); }
};

/** Reader that obtains raw data from a file device.
 */
class FileReader : public TReader<StreamReader, ReaderDevices::File> {
public:
    using Ref = boost::shared_ptr<FileReader>;
    using Super = TReader<StreamReader, ReaderDevices::File>;

    /** Factory method that creates a new FileReader object.

        \param bufferSize initial buffer size to use for the creation buffer

        \return new FileReader object
    */
    static Ref Make(size_t bufferSize = ACE_DEFAULT_CDR_BUFSIZE)
    {
        Ref ref(new FileReader(bufferSize));
        return ref;
    }

    /** Constructor for new reader

        \param bufferSize starting size of read buffer
    */
    FileReader(size_t bufferSize = ACE_DEFAULT_CDR_BUFSIZE) : Super(bufferSize) {}
};

/** Reader that obtains raw data from a socket device.
 */
class TCPSocketReader : public TReader<StreamReader, ReaderDevices::TCPSocket> {
public:
    using Ref = boost::shared_ptr<TCPSocketReader>;
    using Super = TReader<StreamReader, ReaderDevices::TCPSocket>;

    /** Factory method that creates a new TCPSocketReader object.

        \param bufferSize initial buffer size to use for the creation buffer.

        \return new TCPSocketReader object
    */
    static Ref Make(size_t bufferSize = ACE_DEFAULT_CDR_BUFSIZE)
    {
        Ref ref(new TCPSocketReader(bufferSize));
        return ref;
    }

    /** Constructor for new reader

        \param bufferSize starting size for read buffer
    */
    TCPSocketReader(size_t bufferSize = ACE_DEFAULT_CDR_BUFSIZE) : Super(bufferSize) {}
};

/** Reader that obtains raw data from a socket device.
 */
class UDPSocketReader : public TReader<DatagramReader, ReaderDevices::UDPSocket> {
public:
    using Ref = boost::shared_ptr<UDPSocketReader>;
    using Super = TReader<DatagramReader, ReaderDevices::UDPSocket>;

    static Ref Make(size_t bufferSize = 64 * 1024)
    {
        Ref ref(new UDPSocketReader(bufferSize));
        return ref;
    }

    /** Constructor for new reader

        \param bufferSize starting size for read buffer
    */
    UDPSocketReader(size_t bufferSize = 64 * 1024) : Super(bufferSize) {}
};

/** Reader that obtains raw data from a socket device.
 */
class MulticastSocketReader : public TReader<DatagramReader, ReaderDevices::MulticastSocket> {
public:
    using Ref = boost::shared_ptr<MulticastSocketReader>;
    using Super = TReader<DatagramReader, ReaderDevices::MulticastSocket>;

    static Ref Make(size_t bufferSize = 64 * 1024)
    {
        Ref ref(new MulticastSocketReader(bufferSize));
        return ref;
    }

    /** Constructor for new reader

        \param bufferSize starting size for read buffer
    */
    MulticastSocketReader(size_t bufferSize = 64 * 1024) : Super(bufferSize) {}
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
