#include "ace/ACE.h"
#include <errno.h>

#include "Logger/Log.h"
#include "Messages/RawVideoHeader.h"
#include "Utils/Format.h"

#include "Decoder.h"
#include "MessageManager.h"
#include "Readers.h"
#include "Task.h"

using namespace SideCar;
using namespace SideCar::IO;
using namespace SideCar::IO::ReaderDevices;

Logger::Log&
Reader::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Reader");
    return log_;
}

Reader::~Reader()
{
    if (available_) {
        available_->release();
        available_ = 0;
    }
}

ACE_Message_Block*
Reader::getMessage()
{
    ACE_Message_Block* tmp = 0;
    std::swap(available_, tmp);
    return tmp;
}

void
Reader::setAvailable(ACE_Message_Block* data)
{
    if (available_) available_->release();
    available_ = data;
}

Logger::Log&
StreamReader::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.StreamReader");
    return log_;
}

StreamReader::StreamReader(size_t bufferSize) : Reader(), building_(0), needed_(0), remaining_(0), needSynch_(false)
{
    Logger::ProcLog log("Reader", Log());
    LOGINFO << bufferSize << std::endl;
    makeIncomingBuffer(bufferSize);
}

StreamReader::~StreamReader()
{
    if (building_) {
        building_->release();
        building_ = 0;
    }
}

void
StreamReader::reset()
{
    remaining_ = needed_ = Preamble::kCDRStreamSize;
    if (building_) building_->reset();
}

void
StreamReader::makeIncomingBuffer(size_t bufferSize)
{
    Logger::ProcLog log("makeIncomingBuffer", Log());
    if (bufferSize < ACE_DEFAULT_CDR_BUFSIZE) bufferSize = ACE_DEFAULT_CDR_BUFSIZE;
    LOGDEBUG << bufferSize << std::endl;

    // Use the faster ACE_Message_Block allocator found in MessageManager.
    //
    building_ = MessageManager::MakeMessageBlock(bufferSize);
    ACE_CDR::mb_align(building_);
    needed_ = sizeof(int16_t) * 2;
    remaining_ = needed_;
    LOGDEBUG << "needed: " << needed_ << std::endl;
}

bool
StreamReader::fetchInput()
{
    static Logger::ProcLog log("fetchInput", Log());

    LOGDEBUG << "remaining: " << remaining_ << std::endl;

    while (remaining_) {
        // Fetch data from the device, storing at the end of the message block.
        //
        LOGDEBUG << "before fetchFromDevice" << std::endl;
        ssize_t fetched = fetchFromDevice(building_->wr_ptr(), remaining_);
        LOGDEBUG << "fetched: " << fetched << std::endl;

        switch (fetched) {
        case -1: // device err
            switch (errno) {
            case EWOULDBLOCK:
            case ETIME: LOGDEBUG << "nothing available - " << Utils::showErrno() << std::endl; return true;
            }
            LOGERROR << "failed to fetch data - " << Utils::showErrno() << std::endl;
            return false;
            break;

        case 0: // device EOF
            return false;
            break;
        };

        // Adjust the write pointer by the number of bytes we read in.
        //
        building_->wr_ptr(fetched);
        remaining_ -= fetched;
        LOGDEBUG << "new remaining: " << remaining_ << std::endl;
        ;

        // If we are have already fetched a Preamble or we don't have enough data to build a message, then stop
        // looping.
        //
        LOGDEBUG << "count so far: " << building_->length() << std::endl;

        if (needed_ > Preamble::kCDRStreamSize || remaining_ > 0) break;

        // If we are looking for the SYNCH word, see if we found it.
        //
        if (needed_ == sizeof(int16_t) * 2) {
            int16_t* ptr = reinterpret_cast<int16_t*>(building_->rd_ptr());
            int16_t magic = *ptr++;
            int16_t byteOrder = *ptr++;
            if (magic != int16_t(Preamble::kMagicTag) || (byteOrder != int16_t(0) && byteOrder != int16_t(0xFFFF))) {
                if (!needSynch_) {
                    LOGERROR << "missing SYNCH" << std::endl;
                    needSynch_ = true;
                }
                building_->reset();
                ACE_CDR::mb_align(building_);
            } else {
                if (needSynch_) {
                    LOGERROR << "found SYNCH" << std::endl;
                    needSynch_ = false;
                }
                needed_ = Preamble::kCDRStreamSize;
            }

            remaining_ = needed_;
            continue;
        }

        // Decode a Preamble and update how many bytes we need for a valid message. If the preamble says 0, then
        // we are done.
        //
        Decoder decoder(building_->duplicate());
        size_t messageSize = decoder.getMessageSize();
        LOGDEBUG << "message size: " << messageSize << std::endl;
        if (messageSize == 0) break;

        // Resize buffer to account for entire message, and recalculate number of bytes we need for a complete
        // message.
        //
        needed_ = Preamble::kCDRStreamSize + messageSize;
        LOGDEBUG << "needed: " << needed_ << " so far: " << building_->length() << std::endl;
        remaining_ = needed_ - building_->length();
        LOGDEBUG << "remaining: " << remaining_ << std::endl;
        if (remaining_ > building_->space()) {
            LOGDEBUG << "expanding buffer - " << remaining_ << " space: " << building_->space() << std::endl;
            ACE_Message_Block* old = building_;
            building_ = MessageManager::MakeMessageBlock(needed_ + ACE_CDR::MAX_ALIGNMENT);
            ACE_CDR::mb_align(building_);
            building_->copy(old->rd_ptr(), old->length());
            old->release();
            assert(remaining_ <= building_->space());
        }

        LOGDEBUG << "remaining: " << remaining_ << std::endl;
    }

    if (remaining_ == 0) {
        setAvailable(building_);
        makeIncomingBuffer(building_->length() + 128);
    }

    LOGDEBUG << "EXIT - remaining: " << remaining_ << std::endl;
    return true;
}

Logger::Log&
DatagramReader::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.DatagramReader");
    return log_;
}

DatagramReader::DatagramReader(size_t maxSize) : building_(0)
{
    makeIncomingBuffer(maxSize);
}

DatagramReader::~DatagramReader()
{
    Logger::ProcLog log("~DatagramReader", Log());
    LOGINFO << this << ' ' << building_ << std::endl;
    if (building_) {
        building_->release();
        building_ = 0;
    }
}

void
DatagramReader::makeIncomingBuffer(size_t size)
{
    static Logger::ProcLog log("makeIncomingBuffer", Log());
    if (size < ACE_DEFAULT_CDR_BUFSIZE) size = ACE_DEFAULT_CDR_BUFSIZE;
    LOGDEBUG << size << std::endl;
    building_ = MessageManager::MakeMessageBlock(size);
    ACE_CDR::mb_align(building_);
}

bool
DatagramReader::fetchInput()
{
    static Logger::ProcLog log("fetchInput", Log());
    LOGINFO << std::endl;
    if (isMessageAvailable()) { LOGERROR << "fetching while message is available" << std::endl; }

    ssize_t fetched = fetchFromDevice(building_->wr_ptr(), building_->size());
    LOGDEBUG << "fetched: " << fetched << std::endl;
    switch (fetched) {
    case -1: // device err
        switch (errno) {
        case EWOULDBLOCK:
        case ETIME: LOGINFO << "nothing available - " << Utils::showErrno() << std::endl; return true;
        }
        LOGERROR << "failed to fetch data - " << Utils::showErrno() << std::endl;
        return false;
        break;

    case 0: // device EOF
        LOGERROR << "EOF" << std::endl;
        return false;
        break;
    }

    building_->wr_ptr(fetched);
    setAvailable(building_);
    makeIncomingBuffer(building_->size());

    return true;
}

Logger::Log&
MulticastSocket::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.ReaderDevices.MulticastSocket");
    return log_;
}

bool
MulticastSocket::join(const ACE_INET_Addr& address)
{
    static Logger::ProcLog log("join", Log());
    ACE_TCHAR buffer[1204];
    address.addr_to_string(buffer, sizeof(buffer));
    LOGINFO << this << " attempting to join " << buffer << std::endl;
    return device_.join(address, 1, 0) != -1;
}
