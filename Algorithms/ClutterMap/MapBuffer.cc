#include <algorithm>
#include <fstream>
#include <iterator>

#include "Logger/Log.h"
#include "Messages/RadarConfig.h"
#include "Utils/VsipVector.h"

#include "MapBuffer.h"

using namespace SideCar::Algorithms;
using namespace SideCar::Messages;

Logger::Log&
MapBuffer::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Algorithms.ClutterMap.MapBuffer");
    return log_;
}

MapBuffer::MapBuffer(const std::string& name, size_t radialPartitionCount, float alpha) :
    name_(name), maxRangeBin_(RadarConfig::GetGateCountMax() + 1), alpha_(0.05),
    partitionScaling_(float(radialPartitionCount) / float(RadarConfig::GetShaftEncodingMax() + 1)),
    buffer_(radialPartitionCount * maxRangeBin_, 0.0), radialCounts_(radialPartitionCount, 0), isLearning_(true)
{
    static Logger::ProcLog log("MapBuffer", Log());
    LOGINFO << "radialPartitionCount: " << radialPartitionCount
            << " shaftMax: " << (RadarConfig::GetShaftEncodingMax() + 1) << " partionScaling: " << partitionScaling_
            << " buffer size: " << buffer_.size() << std::endl;
}

MapBuffer::~MapBuffer()
{
    ;
}

Video::Ref
MapBuffer::add(const Video::Ref& msg)
{
    static Logger::ProcLog log("add", Log());

    size_t radialIndex = size_t(::floor(msg->getShaftEncoding() * partitionScaling_));
    LOGINFO << "encoding: " << msg->getShaftEncoding() << " index: " << radialIndex << std::endl;
    if (radialIndex >= radialCounts_.size()) { LOGERROR << "radialIndex is too big!" << std::endl; }

    std::vector<float>::iterator p = buffer_.begin();
    p += radialIndex * maxRangeBin_;
    Video::const_iterator v = msg->begin();

    size_t limit = maxRangeBin_;
    if (msg->size() < limit) limit = msg->size();

    if (isLearning_) {
        for (size_t index = 0; index < limit; ++index) *p++ += *v++;
        radialCounts_[radialIndex] += 1;
        return msg;
    }

    Video::Ref out(Messages::Video::Make(name_, msg));

    float oneMinusAlpha_ = 1.0 - alpha_;
    size_t radialCount = radialCounts_[radialIndex];

    if (radialCount > 0) {
        radialCounts_[radialIndex] = 0;
        for (size_t index = 0; index < limit; ++index) {
            float tmp = *p / radialCount;
            out->push_back(*v - tmp);
            *p++ = tmp * oneMinusAlpha_ + *v++ * alpha_;
        }
    } else {
        for (size_t index = 0; index < limit; ++index) {
            float tmp = *p;
            out->push_back(*v - tmp);
            *p++ = tmp * oneMinusAlpha_ + *v++ * alpha_;
        }
    }

    return out;
}

void
MapBuffer::freeze()
{
    Logger::ProcLog log("freeze", Log());
    LOGWARNING << std::endl;
    isLearning_ = false;
}

bool
MapBuffer::load(std::istream& is)
{
    Logger::ProcLog log("load", Log());
    LOGINFO << buffer_.size() << std::endl;

    std::vector<float>::iterator p = buffer_.begin();
    for (size_t index = 0; index < buffer_.size(); ++index) {
        if (!(is >> *p++)) return false;
    }

    std::vector<int>::iterator c = radialCounts_.begin();
    for (size_t index = 0; index < radialCounts_.size(); ++index) {
        if (!(is >> *c++)) return false;
    }

    isLearning_ = false;

    return true;
}

bool
MapBuffer::save(std::ostream& os)
{
    Logger::ProcLog log("load", Log());
    LOGINFO << buffer_.size() << std::endl;
    std::copy(buffer_.begin(), buffer_.end(), std::ostream_iterator<float>(os, "\n"));
    std::copy(radialCounts_.begin(), radialCounts_.end(), std::ostream_iterator<float>(os, "\n"));
    return os.good();
}
