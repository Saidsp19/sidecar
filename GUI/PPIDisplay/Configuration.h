#ifndef SIDECAR_GUI_PPIDISPLAY_CONFIGURATION_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_CONFIGURATION_H

#include "GUI/PresetManager.h"

namespace Utils {
class SineCosineLUT;
}

namespace SideCar {
namespace GUI {

class ChannelImaging;
class ChannelSelectorWindow;
class ChannelSetting;
class OnOffSettingsBlock;
class PhantomCursorImaging;
class BugPlotEmitterSettings;
class QSliderSetting;
class RangeRingsImaging;
class RangeTruthsImaging;
class SampleImaging;
class TargetPlotImaging;
class VideoSampleCountTransform;

namespace PPIDisplay {

class BackgroundImageSettings;
class ConfigurationWindow;
class ControlsWindow;
class DecaySettings;
class HistorySettings;
class PlotPositionFunctor;
class VideoImaging;
class ViewSettings;

class Configuration : public PresetManager {
public:
    Configuration(ChannelSelectorWindow* gui1, ConfigurationWindow* gui2, ControlsWindow* gui3);

    ChannelSetting* getVideoChannel() const { return videoChannel_; }

    ChannelSetting* getBinaryChannel() const { return binaryChannel_; }

    ChannelSetting* getExtractionsChannel() const { return extractionsChannel_; }

    ChannelSetting* getRangeTruthsChannel() const { return rangeTruthsChannel_; }

    ChannelSetting* getBugPlotsChannel() const { return bugPlotsChannel_; }

    BackgroundImageSettings* getBackgroundImageSettings() const { return backgroundImageSettings_; }

    DecaySettings* getDecaySettings() const { return decaySettings_; }

    ViewSettings* getViewSettings() const { return viewSettings_; }

    VideoImaging* getVideoImaging() const { return videoImaging_; }

    PhantomCursorImaging* getPhantomCursorImaging() const { return phantomCursorImaging_; }

    QSliderSetting* getGainSetting() const { return gainSetting_; }

    QSliderSetting* getCutoffMinSetting() const { return cutoffMinSetting_; }

    QSliderSetting* getCutoffMaxSetting() const { return cutoffMaxSetting_; }

    VideoSampleCountTransform* getVideoSampleCountTransform() const { return videoSampleCountTransform_; }

    SampleImaging* getBinaryImaging() const { return binaryImaging_; }

    TargetPlotImaging* getExtractionsImaging() const { return extractionsImaging_; }

    RangeTruthsImaging* getRangeTruthsImaging() const { return rangeTruthsImaging_; }

    TargetPlotImaging* getBugPlotsImaging() const { return bugPlotsImaging_; }

    ChannelImaging* getRangeMapImaging() const { return rangeMapImaging_; }

    RangeRingsImaging* getRangeRingsImaging() const { return rangeRingsImaging_; }

    HistorySettings* getHistorySettings() const { return historySettings_; }

    OnOffSettingsBlock* getShowCursorPositionSetting() const { return showCursorPositionSetting_; }

    OnOffSettingsBlock* getShowPhantomCursorSetting() const { return showPhantomCursorSetting_; }

    BugPlotEmitterSettings* getBugPlotEmitterSettings() const { return bugPlotEmitterSettings_; }

    Utils::SineCosineLUT* getSineCosineLUT() const { return sineCosineLUT_; }

private:
    ChannelSetting* videoChannel_;
    ChannelSetting* binaryChannel_;
    ChannelSetting* extractionsChannel_;
    ChannelSetting* rangeTruthsChannel_;
    ChannelSetting* bugPlotsChannel_;
    BackgroundImageSettings* backgroundImageSettings_;
    DecaySettings* decaySettings_;
    HistorySettings* historySettings_;
    ViewSettings* viewSettings_;
    VideoImaging* videoImaging_;
    QSliderSetting* gainSetting_;
    QSliderSetting* cutoffMinSetting_;
    QSliderSetting* cutoffMaxSetting_;
    VideoSampleCountTransform* videoSampleCountTransform_;
    SampleImaging* binaryImaging_;
    PlotPositionFunctor* plotPositionFunctor_;
    TargetPlotImaging* extractionsImaging_;
    RangeTruthsImaging* rangeTruthsImaging_;
    TargetPlotImaging* bugPlotsImaging_;
    ChannelImaging* rangeMapImaging_;
    RangeRingsImaging* rangeRingsImaging_;
    PhantomCursorImaging* phantomCursorImaging_;
    OnOffSettingsBlock* showCursorPositionSetting_;
    OnOffSettingsBlock* showPhantomCursorSetting_;
    BugPlotEmitterSettings* bugPlotEmitterSettings_;
    Utils::SineCosineLUT* sineCosineLUT_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

#endif
