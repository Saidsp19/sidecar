#ifndef SIDECAR_GUI_MASTER_RADARSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_RADARSETTINGS_H

#include "GUI/BoolSetting.h"
#include "GUI/DoubleSetting.h"
#include "GUI/SettingsBlock.h"
#include "GUI/StringSetting.h"

namespace SideCar {
namespace GUI {
namespace Master {

class MainWindow;

class RadarSettings : public SettingsBlock {
    Q_OBJECT
    using Super = SettingsBlock;

public:
    RadarSettings(BoolSetting* transmitting, DoubleSetting* frequency, BoolSetting* rotating, DoubleSetting* rate,
                  BoolSetting* drfmOn, StringSetting* drfmConfig);

    bool isTransmitting() const { return transmitting_->getValue(); }

    double getFrequency() const { return frequency_->getValue(); }

    bool isRotating() const { return rotating_->getValue(); }

    double getRotationRate() const { return rotationRate_->getValue(); }

    bool isDRFMOn() const { return drfmOn_->getValue(); }

    const QString& getDRFMConfig() const { return drfmConfig_->getValue(); }

private:
    BoolSetting* transmitting_;
    DoubleSetting* frequency_;
    BoolSetting* rotating_;
    DoubleSetting* rotationRate_;
    BoolSetting* drfmOn_;
    StringSetting* drfmConfig_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
