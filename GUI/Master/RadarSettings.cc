#include "RadarSettings.h"

using namespace SideCar::GUI::Master;

RadarSettings::RadarSettings(BoolSetting* transmitting,
                             DoubleSetting* frequency, BoolSetting* rotating,
                             DoubleSetting* rotationRate,
                             BoolSetting* drfmOn, StringSetting* drfmConfig)
    : Super(), transmitting_(transmitting), frequency_(frequency),
      rotating_(rotating), rotationRate_(rotationRate), drfmOn_(drfmOn),
      drfmConfig_(drfmConfig)
{
    add(transmitting);
    add(frequency);
    add(rotating);
    add(rotationRate);
    add(drfmOn);
    add(drfmConfig);
}
