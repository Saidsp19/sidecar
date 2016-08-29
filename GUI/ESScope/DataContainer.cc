#include <limits>

#include "DataContainer.h"

using namespace SideCar::GUI::ESScope;

const DataContainer::DatumType DataContainer::minValue_ = 
    std::numeric_limits<DataContainer::DatumType>::min();

DataContainer::DatumType
DataContainer::GetMinValue()
{
    return minValue_;
}

void
DataContainer::clear()
{
    data_.clear();
}
