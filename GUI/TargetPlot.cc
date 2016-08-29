#include <cmath>

#include "Messages/BugPlot.h"
#include "Messages/Extraction.h"
#include "Messages/TSPI.h"

#include "TargetPlot.h"

using namespace SideCar::GUI;

TargetPlot::TargetPlot(const Messages::BugPlot& msg)
    : range_(msg.getRange()), azimuth_(msg.getAzimuth()), elevation_(msg.getElevation()),
      x_(range_ * ::sin(azimuth_)), y_(range_ * ::cos(azimuth_)),
      tag_(QString::fromStdString(msg.getTag())), age_()
{
    age_.start();
}

TargetPlot::TargetPlot(const Messages::Extraction& extraction, const QString& tag)
    : range_(extraction.getRange()), azimuth_(extraction.getAzimuth()), elevation_(extraction.getElevation()),
      x_(extraction.getX()), y_(extraction.getY()), tag_(tag), age_()
{
    age_.start();
}

TargetPlot::TargetPlot(const Messages::TSPI& rangeTruth)
    : range_(rangeTruth.getRange() / 1000.0), azimuth_(rangeTruth.getAzimuth()),
      elevation_(rangeTruth.getElevation()), x_(range_ * ::sin(azimuth_)), y_(range_ * ::cos(azimuth_)),
      tag_(QString::fromStdString(rangeTruth.getTag())), age_()
{
    age_.start();
}
