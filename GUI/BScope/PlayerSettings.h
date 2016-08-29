#ifndef SIDECAR_GUI_BSCOPE_PLAYERSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_PLAYERSETTINGS_H

#include <cmath>

#include "GUI/QCheckBoxSetting.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/QSpinBoxSetting.h"
#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {
namespace BScope {

class PlayerSettings : public SettingsBlock
{
    Q_OBJECT
    using Super = SettingsBlock;
public:

    PlayerSettings(QComboBoxSetting* playbackRate,
                   QSpinBoxSetting* scalingPower, QCheckBoxSetting* looping);

    void connectPlayer(QComboBox* playbackRate, QCheckBox* loop);

    int getPlaybackRate() const
	{ return GetPlaybackRate(playbackRate_->getValue()); }

    int getScalingPower() const { return scalingPower_->getValue(); }

    double getScale() const
	{ return GetScale(scalingPower_->getValue()); }

    bool isLooping() const { return looping_->getValue(); }

    void changeScalingPower(int change);

signals:

    void playbackRateChanged(int msecs);

    void scaleChanged(double scale);

private slots:

    void changePlaybackRate(int index);

    void changeScale(int power);

private:

    static int GetPlaybackRate(int index)
	{ return int(::rint(1000 * ::pow(2.0, index - 2))); }

    static double GetScale(int power)
	{ return ::pow(1.25, power); }

    QComboBoxSetting* playbackRate_;
    QSpinBoxSetting* scalingPower_;
    QCheckBoxSetting* looping_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
