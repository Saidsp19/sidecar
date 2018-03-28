#include "Logger/Log.h"

#include "GatherWriter.h"

using namespace SideCar::IO;

Logger::Log&
GatherWriter::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.IO.GatherWriter");
    return log_;
}

GatherWriter::GatherWriter(Writer& writer) :
    writer_(writer), sizeLimit_(0), countLimit_(0), first_(0), last_(0), size_(0), count_(0), ok_(true)
{
    ;
}

GatherWriter::~GatherWriter()
{
    if (count_) flush();
}

void
GatherWriter::setSizeLimit(size_t sizeLimit)
{
    Logger::ProcLog log("setSizeLimit", Log());
    LOGINFO << sizeLimit << std::endl;
    sizeLimit_ = sizeLimit;
    if (needFlush()) flush();
}

void
GatherWriter::setCountLimit(size_t countLimit)
{
    Logger::ProcLog log("setCountLimit", Log());
    LOGINFO << countLimit << std::endl;
    countLimit_ = countLimit;
    if (needFlush()) flush();
}

bool
GatherWriter::needFlush() const
{
    if (!count_) return false;
    if (!countLimit_ && !sizeLimit_) return true;
    return (countLimit_ && count_ >= countLimit_) || (sizeLimit_ && size_ >= sizeLimit_);
}

void
GatherWriter::flush()
{
    static Logger::ProcLog log("flush", Log());
    LOGINFO << count_ << ' ' << size_ << std::endl;
    if (count_) {
        ok_ = writer_.writeEncoded(count_, first_);
        if (!ok_) { LOGERROR << "failed to write encoded data - " << errno << " - " << strerror(errno) << std::endl; }
        first_ = 0;
        last_ = 0;
        size_ = 0;
        count_ = 0;
    }
}

bool
GatherWriter::add(ACE_Message_Block* data)
{
    static Logger::ProcLog log("write", Log());
    LOGINFO << ok_ << ' ' << data << std::endl;

    if (!ok_) {
        return false;
    } else if (!data) {
        LOGERROR << "given NULL pointer" << std::endl;
        return false;
    } else if (data->total_length() > 40 * 1024) {
        LOGERROR << "message too large - " << data->total_length() << std::endl;
        data->release();
        return false;
    }

    if (++count_ == 1) {
        first_ = data;
        last_ = data;
    } else {
        last_->next(data);
        last_ = data;
    }

    size_ += data->total_length();
    LOGDEBUG << "count: " << count_ << " size: " << size_ << std::endl;

    if (needFlush()) { flush(); }

    return ok_;
}
