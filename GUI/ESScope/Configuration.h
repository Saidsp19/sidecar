#ifndef SIDECAR_GUI_ESSCOPE_CONFIGURATION_H // -*- C++ -*-
#define SIDECAR_GUI_ESSCOPE_CONFIGURATION_H

#include "GUI/PresetManager.h"
#include "GUI/QCheckBoxSetting.h"

namespace SideCar {
namespace GUI {

class BoolSetting;
class BugPlotEmitterSettings;
class ChannelImaging;
class ChannelSelectorWindow;
class ChannelSetting;
class ControlsWindow;
class IntSetting;
class OnOffSettingsBlock;
class PhantomCursorImaging;
class QSliderSetting;
class SampleImaging;
class RangeTruthsImaging;
class TargetPlotImaging;
class VideoSampleCountTransform;

namespace ESScope {

class ConfigurationWindow;
class FramesListSettings;
class GridImaging;
class History;
class PlayerSettings;
class RadarSettings;
class VideoImaging;

/** Configuration settings for the ESScope application.
*/
class Configuration : public PresetManager
{
public:

    Configuration(ChannelSelectorWindow* gui1, ConfigurationWindow* gui2, ControlsWindow* gui3);

    ChannelSetting* getVideoChannel() const { return videoChannel_; }

    ChannelSetting* getExtractionsChannel() const { return extractionsChannel_; }

    ChannelSetting* getRangeTruthsChannel() const { return rangeTruthsChannel_; }

    ChannelSetting* getBugPlotsChannel() const { return bugPlotsChannel_; }

    VideoImaging* getVideoImaging() const { return videoImaging_; }

    QSliderSetting* getGainSetting() const { return gainSetting_; }

    QSliderSetting* getCutoffMinSetting() const { return cutoffMinSetting_; }

    QSliderSetting* getCutoffMaxSetting() const { return cutoffMaxSetting_; }

    VideoSampleCountTransform* getVideoSampleCountTransform() const { return videoSampleCountTransform_; }

    TargetPlotImaging* getExtractionsImaging() const { return extractionsImaging_; }

    RangeTruthsImaging* getRangeTruthsImaging() const { return rangeTruthsImaging_; }

    TargetPlotImaging* getBugPlotsImaging() const { return bugPlotsImaging_; }

    GridImaging* getGridImaging() const { return gridImaging_; }

    PhantomCursorImaging* getPhantomCursorImaging() const { return phantomCursorImaging_; }

    PlayerSettings* getPlayerSettings() const { return playerSettings_; }

    FramesListSettings* getFramesListSettings() const { return framesListSettings_; }

    OnOffSettingsBlock* getShowPhantomCursorSetting() const { return showPhantomCursorSetting_; }

    BugPlotEmitterSettings* getBugPlotEmitterSettings() const { return bugPlotEmitterSettings_; }

    RadarSettings* getRadarSettings() const { return radarSettings_; }

    History* getHistory() const { return history_; }

    bool usingAllynHack() const { return allynHack_->getValue(); }

private:
    ChannelSetting* videoChannel_;
    ChannelSetting* extractionsChannel_;
    ChannelSetting* rangeTruthsChannel_;
    ChannelSetting* bugPlotsChannel_;
    VideoImaging* videoImaging_;
    QSliderSetting* gainSetting_;
    QSliderSetting* cutoffMinSetting_;
    QSliderSetting* cutoffMaxSetting_;
    VideoSampleCountTransform* videoSampleCountTransform_;
    TargetPlotImaging* extractionsImaging_;
    RangeTruthsImaging* rangeTruthsImaging_;
    TargetPlotImaging* bugPlotsImaging_;
    GridImaging* gridImaging_;
    PhantomCursorImaging* phantomCursorImaging_;
    PlayerSettings* playerSettings_;
    FramesListSettings* framesListSettings_;
    OnOffSettingsBlock* showPhantomCursorSetting_;
    BugPlotEmitterSettings* bugPlotEmitterSettings_;
    RadarSettings* radarSettings_;
    History* history_;
    QCheckBoxSetting* allynHack_;
};

} // end namespace ESScope
} // end namespace GUI
} // end namespace SideCar

#endif
