#ifndef SIDECAR_GUI_HEALTHANDSTATUS_PLOTDATAPOINTDEQUE_H // -*- C++ -*-
#define SIDECAR_GUI_HEALTHANDSTATUS_PLOTDATAPOINTDEQUE_H

#include <deque>

#include "Time/TimeStamp.h"

namespace SideCar {
namespace GUI {
namespace HealthAndStatus {

struct PlotDataPoint {

    PlotDataPoint(double s) : sample(s), when(Time::TimeStamp::Now()) {}

    double sample;
    Time::TimeStamp when;
};

class PlotDataPointDeque : public std::deque<PlotDataPoint>
{
    std::deque<PlotDataPoint> Super;
public:

    PlotDataPointDeque() : Super() {}
};

} // end namespace HealthAndStatus
} // end namespace GUI
} // end namespace SideCar

#endif
