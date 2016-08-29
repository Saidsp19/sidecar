#include <errno.h>
#include "ace/CDR_Stream.h"
#include "ace/OS.h"

#include "Logger/Log.h"

#include "MessageManager.h"
#include "Task.h"
#include "Writers.h"

using namespace SideCar;
using namespace SideCar::IO;

Logger::Log&
WriterDevices::MulticastSocket::Log()
{
    static Logger::Log& log = Logger::Log::Find("MulticastSocket");
    return log;
}

WriterDevices::MulticastSocket::MulticastSocket()
    : device_()
{
    ;
}

ssize_t
WriterDevices::MulticastSocket::writeToDevice(const iovec* iov, int count)
{
    static Logger::ProcLog log("writeToDevice", Log());

    ACE_INET_Addr addr;
    device_.get_local_addr(addr);
    ssize_t rc = device_.ACE_SOCK_Dgram::send(iov, count, addr);

    if (rc == -1) {
	LOGERROR << "writeToDevice failed - " << errno << " - " << ::strerror(errno) << std::endl;
	ACE_INET_Addr addr;
	device_.get_local_addr(addr);
	LOGERROR << "local addr: " << Utils::INETAddrToString(addr) << std::endl;
    }

    return rc;
}

Logger::Log&
WriterDevices::UDPSocket::Log()
{
    static Logger::Log& log = Logger::Log::Find("UDPSocket");
    return log;
}

WriterDevices::UDPSocket::UDPSocket()
    : device_(), remoteAddress_()
{
    ;
}

ssize_t
WriterDevices::UDPSocket::writeToDevice(const iovec* iov, int count)
{
    static Logger::ProcLog log("writeToDevice", Log());

    ssize_t rc = device_.send(iov, count, remoteAddress_);
    if (rc == -1) {
	LOGERROR << "writeToDevice failed - " << errno << " - " << ::strerror(errno) << std::endl;
	LOGERROR << "addr: " << Utils::INETAddrToString(remoteAddress_) << std::endl;
    }

    return rc;
}

size_t
IOVVector::push_back(const ACE_Message_Block* data)
{
    size_t sum = 0;
    while (data) {
	const ACE_Message_Block* chain = data;
	data = data->next();
	do {
	    if (chain->length()) {
		sum += chain->length();
		Super::push_back(IOV(chain->rd_ptr(), chain->length()));
	    }
	    chain = chain->cont();
	} while (chain);
    }

    return sum;
}

Logger::Log&
Writer::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Writer");
    return log_;
}

bool
Writer::write(const MessageManager& mm)
{
    return writeEncoded(1, mm.getEncoded());
}

bool
Writer::writeEncoded(size_t messageCount, ACE_Message_Block* data)
{
    static Logger::ProcLog log("writeEncoded", Log());

    IOVVector iovs;

    size_t size = iovs.push_back(data);
    ssize_t remaining = size;
    LOGTIN << "sending " << remaining << " bytes" << std::endl;

    IOVVector::const_iterator pos = iovs.begin();
    IOVVector::const_iterator end = iovs.end();
    while (pos != end && ! isClosing()) {

	// Don't send too many iovec elements at one time.
	//
	int count = end - pos;
	if (count > ACE_IOV_MAX) count = ACE_IOV_MAX;
	LOGDEBUG << "writing " << count << " iovec entries" << std::endl;

    again:

	errno = 0;
	ssize_t rc = writeToDevice(&*pos, count);
	LOGDEBUG << "writeToDevice: rc=" << rc << " remaining=" << remaining << std::endl;
	if (rc == -1) {
	    LOGERROR << "writeToDevice failed - " << errno << " - " << ::strerror(errno) << std::endl;
	    if (errno == EAGAIN && ! isClosing()) goto again;
	    lastError_ = errno;
	    break;
	}

	remaining -= rc;
	pos += count;
    }

    // We are responsible for reducing reference counts to all data objects given to us, even if there was an
    // error above.
    //
    while (data) {
	ACE_Message_Block* next = data->next();
	data->release();
	data = next;
    }

    LOGTOUT << "FT"[remaining == 0] << std::endl;
    return remaining == 0;
}
