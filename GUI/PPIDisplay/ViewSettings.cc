#include <cmath>

#include "GUI/LogUtils.h"

#include "ViewSettings.h"

using namespace SideCar::GUI::PPIDisplay;

Logger::Log&
ViewSettings::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.ViewSettings");
    return log_;
}

ViewSettings::ViewSettings(DoubleSetting* rangeMaxMax, DoubleSetting* rangeMax, DoubleSetting* x, DoubleSetting* y,
                           IntSetting* zoomPower, DoubleSetting* zoomFactor) :
    Super(),
    rangeFactor_(0.0), rangeMaxMax_(rangeMaxMax), rangeMax_(rangeMax), x_(x), y_(y), zoomPower_(zoomPower),
    zoomFactor_(zoomFactor)
{
    connect(rangeMaxMax_, SIGNAL(valueChanged(double)), this, SIGNAL(rangeMaxMaxChanged(double)));
    connect(rangeMax, SIGNAL(valueChanged(double)), this, SIGNAL(rangeMaxChanged(double)));

    add(x);
    add(y);

    connect(zoomPower, SIGNAL(valueChanged(int)), this, SLOT(updateZoom()));
    connect(zoomFactor, SIGNAL(valueChanged(double)), this, SLOT(updateZoom()));

    updateZoom();
}

void
ViewSettings::moveTo(double x, double y)
{
    x_->setValue(x);
    y_->setValue(y);
}

void
ViewSettings::updateZoom()
{
    static Logger::ProcLog log("updateZoom", Log());
    zoom_ = getZoom(zoomPower_->getValue());
    LOGINFO << "zoomFactor: " << zoomFactor_->getValue() << " zoomPower: " << zoomPower_->getValue()
            << " zoom: " << zoom_ << std::endl;
    postSettingChanged();
}

void
ViewSettings::setZoomPower(int value)
{
    static Logger::ProcLog log("setZoomPower", Log());
    LOGINFO << "value: " << value << std::endl;
    if (value < 0)
        value = 0;
    else if (value > 20)
        value = 20;
    zoomPower_->setValue(value);
}

void
ViewSettings::setRangeFactorAndMax(double rangeFactor, double rangeMaxMax)
{
    rangeFactor_ = rangeFactor;
    rangeMaxMax_->setValue(rangeMaxMax);
    if (rangeMaxMax < rangeMax_->getValue()) { rangeMax_->setValue(rangeMaxMax); }
}

void
ViewSettings::reset()
{
    x_->setValue(0.0);
    y_->setValue(0.0);
    zoomPower_->setValue(0);
}

double
ViewSettings::getZoom(int power) const
{
    return ::pow(zoomFactor_->getValue(), power);
}
