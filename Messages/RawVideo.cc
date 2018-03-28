#include <iomanip>
#include <netinet/in.h>

#include "Logger/Log.h"

#include "RawVideo.h"
#include "Video.h"

using namespace SideCar::Messages;

MetaTypeInfo RawVideo::metaTypeInfo_(MetaTypeInfo::Value::kRawVideo, "RawVideo", 0, 0);

Logger::Log&
RawVideo::Log()
{
    static Logger::Log& log = Logger::Log::Find("SideCar.Messages.RawVideo");
    return log;
}

const MetaTypeInfo&
RawVideo::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

RawVideo::Ref
RawVideo::Make(const std::string& producer, ACE_Message_Block* data)
{
    static Logger::ProcLog log("Make", Log());
    LOGINFO << producer << " data: " << data << std::endl;

    // Fetch the raw msgDesc value. It is always in network-byte order.
    //
    VMEDataMessage* raw = reinterpret_cast<VMEDataMessage*>(data->rd_ptr());
    uint32_t msgDesc = ntohl(raw->header.msgDesc);
    bool isLittleEndian = (msgDesc & VMEHeader::kEndianessMask);

    // Create an input CDR stream to fetch binary data from. Handles the byte-swapping for us when necessary.
    //
    ACE_InputCDR cdr(data, isLittleEndian ? 1 : 0);

    // Extract the VME message header so we can get its PRI sequence number.
    //
    VMEHeader vmeHeader;
    vmeHeader.load(cdr);
    Ref ref(new RawVideo(producer, data, vmeHeader.pri));

    return ref;
}

RawVideo::Ref
RawVideo::Make(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("Make", Log());
    Ref ref(new RawVideo);
    ref->load(cdr);
    return ref;
}

RawVideo::~RawVideo()
{
    if (raw_) raw_->release();
}

ACE_InputCDR&
RawVideo::load(ACE_InputCDR& cdr)
{
    static Logger::ProcLog log("load", Log());
    LOGINFO << "size: " << cdr.length() << std::endl;
    Super::load(cdr);
    if (raw_) raw_->release();
    raw_ = new ACE_Message_Block(cdr.length());
    raw_->copy(cdr.rd_ptr(), cdr.length());
    return cdr;
}

ACE_OutputCDR&
RawVideo::write(ACE_OutputCDR& cdr) const
{
    static Logger::ProcLog log("write", Log());
    LOGINFO << "size: " << raw_->length() << std::endl;
    Super::write(cdr);
    cdr.write_octet_array_mb(raw_);
    return cdr;
}

std::ostream&
RawVideo::printData(std::ostream& os) const
{
    size_t count = raw_->length();
    os << "Size: " << count << '\n';
    const char* ptr = raw_->rd_ptr();
    os << std::hex;
    while (count) {
        for (int index = 0; index < 26 && count; ++index, --count) os << std::setw(2) << int(*ptr++) << ' ';
        os << '\n';
    }
    return os << std::dec;
}

Video::Ref
RawVideo::convert(const std::string& producer) const
{
    static Logger::ProcLog log("convert", Log());
    LOGINFO << std::endl;

    // ACE_CDR_BYTE_ORDER == 0 for PowerPC (big-endian) ACE_CDR_BYTE_ORDER == 1 for Intel (little-endian)
    //
    if (!raw_) {
        LOGERROR << "NULL raw_ attribute" << std::endl;
        return Video::Ref();
    }

    // Fetch the raw msgDesc value. It is always in network-byte order.
    //
    VMEDataMessage* rawPtr = reinterpret_cast<VMEDataMessage*>(raw_->rd_ptr());
    uint32_t msgDesc = ntohl(rawPtr->header.msgDesc);
    bool isLittleEndian = (msgDesc & VMEHeader::kEndianessMask);

    // Create an input CDR stream to fetch binary data from. Handles the byte-swapping for us when necessary.
    //
    ACE_InputCDR cdr(raw_, isLittleEndian ? 1 : 0);

    // Extract the VME message header, but since the msgDesc is always in network byte-order (big-endian), we
    // revert to the raw value, regardless of endianess.
    //
    VMEDataMessage vme;
    vme.load(cdr);
    vme.header.msgDesc = msgDesc;
    LOGDEBUG << vme << std::endl;

    // Adjust the count if we have both I and Q samples.
    //
    size_t count = vme.numSamples;
    bool isComplex = vme.header.getFormat() == VMEHeader::kPackedIQ;
    if (isComplex) count *= 2;

    // Make sure we have enough bytes in the input CDR stream. If not, then process what we can, but raise a
    // stink about it. Most likely, we have a endianess or format issue with the VME code.
    //
    size_t available = cdr.length() / 2;
    if (count > available) {
        LOGERROR << vme << std::hex << msgDesc << std::dec << " numSamples (" << count << ") != available ("
                 << available << ")" << std::endl;
        count = available;
        LOGDEBUG << "reduced count: " << count << std::endl;
    }

    LOGDEBUG << "count: " << count << std::endl;

    if (!isComplex) {
        Video::Ref ref(Video::Make(producer, vme, count));

        // The VME system will emit data in two formats: inverted * 4 integers, and normal integers. The
        // inverted * 4 values require a bit-wise inversion followed by a right shift of 2 bits (dividing by 4).
        // The other format simply requires us to take the values from the ACE CDR, which handles any
        // byte-swapping that may be necessary. NOTE: the VME also emits kPackedReal data that is NOT inverted *
        // 4; thus the additional check for the data being in little-endian format.
        //
        if ((vme.header.getFormat() == VMEHeader::kPackedReal && isLittleEndian) ||
            vme.header.getFormat() == VMEHeader::kUnpackedReal) {
            LOGDEBUG << "handling kPackedReal/kUnpackedReal" << std::endl;
            Video::DatumType value;
            Video::Container& data(ref->getData());
            while (count--) {
                cdr >> value;
                data.push_back((~value) >> 2);
            }
        } else {
            ref->resize(count);
            cdr.read_short_array(&ref[0], count);
        }
        return ref;
    } else {
        Video::Ref ref(Video::Make(producer, vme, count));
        ref->resize(count);
        cdr.read_short_array(reinterpret_cast<int16_t*>(&ref[0]), count);
        return ref;
    }
}
