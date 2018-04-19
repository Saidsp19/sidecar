#include <cmath>

#include "Logger/Log.h"
#include "SineCosineLUT.h"

using namespace Utils;

Logger::Log&
SineCosineLUT::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Utils.SineCosineLUT");
    return log_;
}

SineCosineLUT*
SineCosineLUT::Make(size_t size)
{
    Logger::ProcLog log("SineCosineLUT::Make", Log());

    if (size % 4 != 0) {
        LOGERROR << "Requested LUT size not a multiple of 4 - " << size << std::endl;
        return 0;
    }

    return new SineCosineLUT(size);
}

SineCosineLUT::SineCosineLUT(size_t size) : values_(size / 4), size_(size)
{
    if (size % 4 != 0) {
        InvalidSize ex;
        ex << "Size not a multiple of 4 - " << size;
        throw ex;
    }

    // Fill in one quadrant.
    //
    double increment = M_PI * 2.0 / size;
    size = values_.size();
    for (auto index = 0; index < size; ++index) {
        auto angle = index * increment;
        values_[index] = SineCosine(::sin(angle), ::cos(angle));
    }
}

void
SineCosineLUT::lookup(size_t index, double& sine, double& cosine) const
{
    while (index >= size_) { index -= size_; }

    auto quadrant = index / values_.size();
    index -= quadrant * values_.size();
    const SineCosine& value(values_[index]);

    switch (quadrant) {
    case 0:
        sine = value.sine;
        cosine = value.cosine;
        return;

    case 1:
        sine = value.cosine;
        cosine = -value.sine;
        return;

    case 2:
        sine = -value.sine;
        cosine = -value.cosine;
        return;

    case 3:
        sine = -value.cosine;
        cosine = value.sine;
        return;
    }
}
