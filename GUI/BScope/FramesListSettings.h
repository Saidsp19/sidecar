#ifndef SIDECAR_GUI_BSCOPE_FRAMESLISTSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_FRAMESLISTSETTINGS_H

#include <cmath>

#include "GUI/QCheckBoxSetting.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/QSpinBoxSetting.h"
#include "GUI/SettingsBlock.h"

namespace SideCar {
namespace GUI {

namespace BScope {

class FramesListSettings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    FramesListSettings(QSpinBoxSetting* scalingPower);

    int getScalingPower() const { return scalingPower_->getValue(); }

    double getScale() const { return GetScale(scalingPower_->getValue()); }

    void changeScalingPower(int change);

signals:

    void scaleChanged(double scale);

private slots:

    void changeScale(int power);

private:
    static double GetScale(int power) { return ::pow(1.25, power); }

    QSpinBoxSetting* scalingPower_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
