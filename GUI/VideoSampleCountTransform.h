#ifndef SIDECAR_GUI_VIDEOSAMPLECOUNTTRANSFORM_H // -*- C++ -*-
#define SIDECAR_GUI_VIDEOSAMPLECOUNTTRANSFORM_H

#include <cmath>

#include "GUI/BoolSetting.h"
#include "GUI/IntSetting.h"
#include "GUI/SettingsBlock.h"

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {

/** Converts sample count information contained in Messages::Video messages into values between 0 and 1,
    inclusive. The mapping takes into account user controls that provide gain and lower-bounds thresholding. Can
    also return values in decibels.

    The gain_ setting is simply a multiplier that is applied to incoming sample
    counts before anything else is done. The ControlsWindow contains a GUI
    widget that manipulates this value.

    The cutoff_ affects how much of the sample range (defined by sampleMin_ and
    sampleMax_) is mapped to the output range 0-1. Its value ranges from
    [0-1.0) and reflects a percentage of the available sample range, starting
    at the low end. A cutoff_value of 0 provides a full mapping, while values >
    0.0 create a threshold greater than sampleMin_, thus reducing the input
    bandwidth:

    \code
    threshold_ = cutoff_ * (sampleMax_ - sampleMin_) + sampleMin_;
    \endcode

    The transform() routine clamps input values to the range [threshold_,
    sampleMax_].
*/
class VideoSampleCountTransform : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    static Logger::Log& Log();

    /** Constructor.

        \param sampleMin smallest sample value in message data

        \param sampleMax largest sample value in message data

        \param gain initial gain value multiplied with samples

        \param cutoffMin initial low-end cutoff value

        \param cutoffMax initial high-end cutoff value

        \param showDecibels initial setting for returning decibel values
    */
    VideoSampleCountTransform(IntSetting* sampleMin, IntSetting* sampleMax, IntSetting* gain, IntSetting* cutoffMin,
                              IntSetting* cutoffMax, BoolSetting* showDecibels);

    /** Transform a sample count value into a value in range [0-1].

        \param sampleCount the value to transform

        \return value in range [0-1]
    */
    double transform(int sampleCount) const;

    int inverseTransform(double value) const;

    /** Call operator to allow transform function in STL constructs.

        \param sampleCount the value to transform

        \return value in range [0-1]
    */
    double operator()(int sampleCount) const { return transform(sampleCount); }

    /** Obtain the current min sample value

        \return min sample
    */
    int getSampleMin() const { return sampleMin_->getValue(); }

    /** Obtain the current max sample value

        \return max sample
    */
    int getSampleMax() const { return sampleMax_->getValue(); }

    /** Obtain the current low-end cutoff value

        \return cutoff value
    */
    double getThresholdMin() const { return thresholdMin_; }

    /** Obtain the current high-end cutoff percentage

        \return cutoff value
    */
    double getThresholdMax() const { return thresholdMax_; }

    /** Determine if values from transform() are in decibels.

        \return true if so
    */
    bool getShowDecibels() const { return showDecibels_->getValue(); }

    void setThresholdMin(int value) { cutoffMin_->setValue(value); }

    void setThresholdMax(int value) { cutoffMax_->setValue(value); }

signals:

    /** Notification sent out when the decibel setting changes.

        \param state new state
    */
    void showingDecibels(bool state);

public slots:

    void setShowDecibels(bool value) { showDecibels_->setValue(value); }

private slots:

    /** Set a new gain value.

        \param gain value to use
    */
    void gainChanged(int gain);

    /** Change the state of returning values in decibels.

        \param value true if so
    */
    void showDecibelsChanged(bool value);

    /** Override of SettingsBlock::emitSettingChanged(). Notification handler invoked to emit the
        settingChanged() signal. Recalculates various transform parameters invoking
        SettingsBlock::emitSettingChanged() to emit the settingChanged() signal.
    */
    void emitSettingChanged();

private:
    /** Recalculate the transform equation parameters.
     */
    void recalculate();

    /** Calculate decibel (dbV) for a given vale

        \param value the value to work on

        \return decibel value
    */
    double decibel(double value) const { return 20.0 * ::log10((value + offset_) * scale_); }

    IntSetting* sampleMin_;
    IntSetting* sampleMax_;
    IntSetting* cutoffMin_;
    IntSetting* cutoffMax_;
    BoolSetting* showDecibels_;

    double gainValue_;
    double thresholdMin_;
    double thresholdMax_;
    double offset_;
    double scale_;
    double normalizer_;
    double minLog_;
    double maxLog_;

    static const double kLogEpsilon;
};

} // end namespace GUI
} // end namespace SideCar

#endif
