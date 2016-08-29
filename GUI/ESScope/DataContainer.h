#ifndef SIDECAR_GUI_ESSCOPE_DATACONTAINER_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_DATACONTAINER_H

#include "Messages/Video.h"

namespace SideCar {
namespace GUI {
namespace ESScope {

class DataContainer 
{
public:
    using DatumType = Messages::Video::DatumType;
    using Container = Messages::Video::Container;

    static DatumType GetMinValue();

    DataContainer() : data_() {}

    DataContainer(size_t count)
	: data_(count, GetMinValue()) {}

    DataContainer(size_t count, DatumType init)
	: data_(count, init) {}

    virtual ~DataContainer() {}

    void resize(size_t size)
	{ data_.resize(size, GetMinValue()); }

    void resize(size_t size, const DatumType& init)
	{ data_.resize(size, init); }

    const Container& getValues() const { return data_; }

    virtual void clear();

    DatumType& operator[](size_t index) { return data_[index]; }

    const DatumType& operator[](size_t index) const { return data_[index]; }

    size_t size() const { return data_.size(); }

    bool empty() const { return data_.empty(); }

protected:

    Container data_;

private:
    static const DatumType minValue_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
