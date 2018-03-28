#ifndef SIDECAR_GUI_BSCOPE_VIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_VIEWSETTINGS_H

#include "GUI/DoubleSetting.h"
#include "GUI/SettingsBlock.h"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {

class IntSetting;

namespace BScope {

/** Collection of settings that relate the the view shown in a PPIWidget. Specifically, a view has horizontal
    and vertical offset for the radar's center, where zero offsets place the radar in the center of the screen;
    a zoom factor that magnifies the data; and a maximum visible range value that limits the samples used for
    the display.
*/
class ViewSettings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    static Logger::Log& Log();

    ViewSettings(IntSetting* azimuthZero, IntSetting* azimuthSpan, DoubleSetting* rangeMin, DoubleSetting* rangeMax);

    void setRangeFactorAndMax(double rangeFactor, double rangeMaxMax);

    double getAzimuthMin() const { return azimuthMin_; }

    double getAzimuthMax() const { return azimuthMax_; }

    double getAzimuthSpan() const { return getAzimuthMax() - getAzimuthMin(); }

    double getRangeMin() const { return rangeMin_->getValue(); }

    double getRangeMax() const { return rangeMax_->getValue(); }

    double getRangeSpan() const { return getRangeMax() - getRangeMin(); }

    double getRangeFactor() const { return rangeFactor_; }

    double getRangeMaxMax() const { return rangeMaxMax_; }

    double normalizedAzimuth(double radians) const;

    double azimuthToParametric(double radians) const;

    double azimuthFromParametric(double value) const;

    double rangeToParametric(double value) const;

    double rangeFromParametric(double value) const;

    bool viewingAzimuth(double radians) const;

    bool viewingRange(double range) const { return range >= rangeMin_->getValue() && range <= rangeMax_->getValue(); }

private slots:

    void recalculateAzimuthMinMax();

private:
    void restore();

    void save();

    IntSetting* azimuthZero_;
    IntSetting* azimuthSpan_;
    DoubleSetting* rangeMin_;
    DoubleSetting* rangeMax_;
    double azimuthMin_;
    double azimuthMax_;
    double rangeFactor_;
    double rangeMaxMax_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
