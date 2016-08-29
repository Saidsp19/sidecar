#ifndef SIDECAR_GUI_SPECTRUM_CONFIGURATION_H // -*- C++ -*-
#define SIDECAR_GUI_SPECTRUM_CONFIGURATION_H

#include "GUI/PresetManager.h"

namespace SideCar {
namespace GUI {

class BoolSetting;
class DoubleSetting;

namespace Spectrum {

class ConfigurationWindow;
class FFTSettings;
class Settings;
class SpectrographImaging;

class Configuration : public PresetManager
{
    Q_OBJECT
    using Super = PresetManager;
public:

    Configuration(ConfigurationWindow* gui2);

    Settings* getSettings() const { return settings_; }

    FFTSettings* getFFTSettings() const { return fftSettings_; }

    SpectrographImaging* getSpectrographImaging() const
	{ return spectrographImaging_; }

    BoolSetting* getAzLatchEnabled() const
	{ return azLatchEnabled_; }

    DoubleSetting* getAzLatchAzimuth() const
	{ return azLatchAzimuth_; }

    BoolSetting* getAzLatchRelatch() const
	{ return azLatchRelatch_; }

private:
    Settings* settings_;
    FFTSettings* fftSettings_;
    SpectrographImaging* spectrographImaging_;
    BoolSetting* azLatchEnabled_;
    DoubleSetting* azLatchAzimuth_;
    BoolSetting* azLatchRelatch_;
};

} // end namespace Spectrum
} // end namespace GUI
} // end namespace SideCar

#endif
