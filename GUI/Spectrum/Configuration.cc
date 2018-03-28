#include "GUI/BoolSetting.h"
#include "GUI/ColorButtonSetting.h"
#include "GUI/IntSetting.h"
#include "GUI/OpacitySetting.h"
#include "GUI/QDoubleSpinBoxSetting.h"
#include "GUI/QGroupBoxSetting.h"

#include "Messages/Video.h"

#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "FFTSettings.h"
#include "Settings.h"
#include "SpectrographImaging.h"

using namespace SideCar::GUI::Spectrum;
using namespace SideCar::Messages;

Configuration::Configuration(ConfigurationWindow* win) :
    Super("Configuration"),
    settings_(new Settings(new QDoubleSpinBoxSetting(this, win->sampleFrequencyValue_),
                           new QComboBoxSetting(this, win->sampleFrequencyScale_),
                           new ChannelSetting(Video::GetMetaTypeInfo().getName(), this),
                           new ColorButtonSetting(this, win->liveColor_), new QCheckBoxSetting(this, win->showGrid_),
                           new QComboBoxSetting(this, win->drawingMode_), new QSpinBoxSetting(this, win->powerMax_))),
    fftSettings_(new FFTSettings(
        new QComboBoxSetting(this, win->fftSize_), new QSpinBoxSetting(this, win->gateStart_),
        new QComboBoxSetting(this, win->windowType_), new QCheckBoxSetting(this, win->zeroPad_),
        new QSpinBoxSetting(this, win->workerThreadCount_), new QSpinBoxSetting(this, win->smoothing_), settings_)),
    spectrographImaging_(new SpectrographImaging(new QGroupBoxSetting(this, win->spectrographVisible_),
                                                 new ColorButtonSetting(this, win->spectrographColor_),
                                                 new QDoubleSpinBoxSetting(this, win->spectrographPointSize_),
                                                 new OpacitySetting(this, win->spectrographOpacity_),
                                                 new QCheckBoxSetting(this, win->spectrographColorMapEnabled_),
                                                 new QComboBoxSetting(this, win->spectrographColorMap_),
                                                 new QDoubleSpinBoxSetting(this, win->spectrographMinCutoff_),
                                                 new QDoubleSpinBoxSetting(this, win->spectrographMaxCutoff_),
                                                 new QSpinBoxSetting(this, win->spectrographHistorySize_)))
{
    azLatchEnabled_ = new BoolSetting(this, "AzLatchEnabled", false, false);
    azLatchAzimuth_ = new DoubleSetting(this, "AzLatchAzimuth", 0.0, false);
    azLatchRelatch_ = new BoolSetting(this, "AzLatchRelatch", true, false);
    restoreAllPresets();
}
