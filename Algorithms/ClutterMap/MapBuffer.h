#ifndef SIDECAR_ALGORITHMS_MAPBUFFER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_MAPBUFFER_H

#include <iosfwd>

#include <vector>
#include <vsip/vector.hpp>

#include "Messages/Video.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace Algorithms {

class MapBuffer {
public:
    using VsipFloatVector = vsip::Vector<float>;
    using VsipFloatVectorBuffer = std::vector<VsipFloatVector*>;

    static Logger::Log& Log();

    MapBuffer(const std::string& name, size_t radialPartitionCount, float alpha);

    ~MapBuffer();

    void setAlpha(float alpha) { alpha_ = alpha; }

    Messages::Video::Ref add(const Messages::Video::Ref& msg);

    void freeze();

    bool isFrozen() const { return !isLearning_; }

    bool load(std::istream& is);

    bool save(std::ostream& os);

private:
    void makeBuffer(size_t radialPartitionCount);

    std::string name_;
    size_t maxRangeBin_;
    float alpha_;
    float partitionScaling_;

    std::vector<float> buffer_;
    std::vector<int> radialCounts_;

    bool isLearning_;
};

} // end namespace Algorithms
} // end namespace SideCar

#endif
