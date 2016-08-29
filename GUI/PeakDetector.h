#ifndef SIDECAR_GUI_PEAKDETECTOR_H // -*- C++ -*-
#define SIDECAR_GUI_PEAKDETECTOR_H

#include "QtCore/QObject"

#include <deque>

namespace SideCar {
namespace GUI {

class PeakDetector : public QObject
{
    Q_OBJECT
public:
    using ValueDeque = std::deque<float>;

    PeakDetector(size_t size, float zero = 0.0, QObject* parent = 0)
	: QObject(parent), values_(size, zero), peakIndex_(0) {}

    void clear(float zero = 0.0);

    float getValue() const { return values_[peakIndex_]; }

    bool add(float value);

    size_t size() const { return values_.size(); }

signals:
    void newPeak(float value);

private:
    ValueDeque values_;
    size_t peakIndex_;
};

} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
