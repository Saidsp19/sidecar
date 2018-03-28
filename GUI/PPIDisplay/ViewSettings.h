#ifndef SIDECAR_GUI_PPIDISPLAY_VIEWSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_VIEWSETTINGS_H

#include "GUI/DoubleSetting.h"
#include "GUI/IntSetting.h"
#include "GUI/SettingsBlock.h"

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {
namespace PPIDisplay {

/** Collection of settings that relate the the view shown in a PPIWidget. Specifically, a view has horizontal
    and vertical offset for the radar's center, where zero offsets place the radar in the center of the screen;
    a zoom factor that magnifies the data; and a maximum visible range value that limits the samples used for
    the display.
*/
class ViewSettings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    /** Obtain Log device for ViewSetting objects

        \return Log device
    */
    static Logger::Log& Log();

    /** Constructor.

        \param rangeMaxMax

        \param rangeMax

        \param x

        \param y

        \param zoomPower

        \param zoomFactor
    */
    ViewSettings(DoubleSetting* rangeMaxMax, DoubleSetting* rangeMax, DoubleSetting* x, DoubleSetting* y,
                 IntSetting* zoomPower, DoubleSetting* zoomFactor);

    /** Obtain the last rangeFactor value given in a setRangeFactorAndMax() call.

        \return range factor
    */
    double getRangeFactor() const { return rangeFactor_; }

    /** Obtain the last max range value given in a setRangeFactorAndMax() call.

        \return range max
    */
    double getRangeMaxMax() const { return rangeMaxMax_->getValue(); }

    /** Obtain the current maximum visible range value. This value should always be less than or equal to
        getRangeMax().

        \return maximum visible range
    */
    double getRangeMax() const { return rangeMax_->getValue(); }

    /** Obtain the current horizontal offset for the radar's center.

        \return horizontal offset value
    */
    float getX() const { return x_->getValue(); }

    /** Obtain the current vertical offset for the radar's center.

        \return vertical offset value
    */
    float getY() const { return y_->getValue(); }

    /** Obtain the current view zoom power setting

        \return zoom power
    */
    int getZoomPower() const { return zoomPower_->getValue(); }

    /** Change the current view zoom power setting

        \param value new value
    */
    void setZoomPower(int value);

    /** Obtain the current magnification for the view.

        \return magnification factor.
    */
    double getZoom() const { return zoom_; }

    /**

        \param power

        \return
    */
    double getZoom(int power) const;

public slots:

    /** Register new range factor/max values. This should only be done when there is an appreciable difference
        between the given rangeFactor and the value of getRangeFactor(), since the rangeMax values may vary by
        one or more gate values due to timing accuracy. NOTE: this routine will change the internal rangeMax_
        value if it is larger than then new max range value.

        \param rangeFactor new gate-to-gate range factor to record

        \param rangeMaxMax new maximum range value to record
    */
    void setRangeFactorAndMax(double rangeFactor, double rangeMaxMax);

    /** Shift the horizontal and/or vertical offsets of the radar center.

        \param x horizontal offset to add

        \param y vertical offset to add
    */
    void shift(double x, double y) { moveTo(x + getX(), y + getY()); }

    /** Set the horizontal and vertical offset of the radar center.

        \param x horizontal offset to use

        \param y vertical offset to use
    */
    void moveTo(double x, double y);

    void reset();

signals:

    /** Notificationn sent out when the maximum range value changes.

        \param value new value
    */
    void rangeMaxMaxChanged(double value);

    /** Notification sent out when the maximum visible range setting changs.

        \param value new value
    */
    void rangeMaxChanged(double value);

private slots:

    /** Notification handler invoked when the zoomPower_ or zoomFactor_ settings change. Calculates a new zoom_
        magnification factor and emits a valueChanged() signal.
    */
    void updateZoom();

private:
    double rangeFactor_;
    DoubleSetting* rangeMaxMax_;
    DoubleSetting* rangeMax_;
    DoubleSetting* x_;
    DoubleSetting* y_;
    IntSetting* zoomPower_;
    DoubleSetting* zoomFactor_;
    double zoom_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
