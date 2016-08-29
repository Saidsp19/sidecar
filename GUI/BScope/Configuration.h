#ifndef SIDECAR_GUI_BSCOPE_CONFIGURATION_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_CONFIGURATION_H

#include "GUI/PresetManager.h"

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
class RangeRingsImaging;
class SampleImaging;
class TargetPlotImaging;
class VideoSampleCountTransform;

namespace BScope {

class ConfigurationWindow;
class FramesListSettings;
class History;
class HistorySettings;
class PlayerSettings;
class PlotPositionFunctor;
class VideoImaging;
class ViewSettings;

class Configuration : public PresetManager
{
public:

    Configuration(ChannelSelectorWindow* gui1, ConfigurationWindow* gui2,
                  ControlsWindow* gui3, History* history);

    ChannelSetting* getVideoChannel() const
	{ return videoChannel_; }

    ChannelSetting* getBinaryChannel() const
	{ return binaryChannel_; }

    ChannelSetting* getExtractionsChannel() const
	{ return extractionsChannel_; }

    ChannelSetting* getRangeTruthsChannel() const
	{ return rangeTruthsChannel_; }

    ChannelSetting* getBugPlotsChannel() const
	{ return bugPlotsChannel_; }

    ViewSettings* getViewSettings() const
	{ return viewSettings_; }

    VideoImaging* getVideoImaging() const
	{ return videoImaging_; }

    QSliderSetting* getGainSetting() const { return gainSetting_; }

    QSliderSetting* getCutoffMinSetting() const { return cutoffMinSetting_; }

    QSliderSetting* getCutoffMaxSetting() const { return cutoffMaxSetting_; }

    VideoSampleCountTransform* getVideoSampleCountTransform() const
	{ return videoSampleCountTransform_; }

    SampleImaging* getBinaryImaging() const
	{ return binaryImaging_; }

    TargetPlotImaging* getExtractionsImaging() const
	{ return extractionsImaging_; }

    TargetPlotImaging* getRangeTruthsImaging() const
	{ return rangeTruthsImaging_; }

    TargetPlotImaging* getBugPlotsImaging() const
	{ return bugPlotsImaging_; }

    ChannelImaging* getRangeMapImaging() const
	{ return rangeMapImaging_; }

    RangeRingsImaging* getRangeRingsImaging() const
	{ return rangeRingsImaging_; }

    PhantomCursorImaging* getPhantomCursorImaging() const
	{ return phantomCursorImaging_; }

    HistorySettings* getHistorySettings() const
	{ return historySettings_; }

    PlayerSettings* getPlayerSettings() const
	{ return playerSettings_; }

    FramesListSettings* getFramesListSettings() const
	{ return framesListSettings_; }

    OnOffSettingsBlock* getShowCursorPositionSetting() const
	{ return showCursorPositionSetting_; }

    OnOffSettingsBlock* getShowPhantomCursorSetting() const
	{ return showPhantomCursorSetting_; }

    BugPlotEmitterSettings* getBugPlotEmitterSettings() const
	{ return bugPlotEmitterSettings_; }

private:
    ChannelSetting* videoChannel_;
    ChannelSetting* binaryChannel_;
    ChannelSetting* extractionsChannel_;
    ChannelSetting* rangeTruthsChannel_;
    ChannelSetting* bugPlotsChannel_;
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
    TargetPlotImaging* rangeTruthsImaging_;
    TargetPlotImaging* bugPlotsImaging_;
    ChannelImaging* rangeMapImaging_;
    RangeRingsImaging* rangeRingsImaging_;
    PhantomCursorImaging* phantomCursorImaging_;
    PlayerSettings* playerSettings_;
    FramesListSettings* framesListSettings_;
    OnOffSettingsBlock* showCursorPositionSetting_;
    OnOffSettingsBlock* showPhantomCursorSetting_;
    BugPlotEmitterSettings* bugPlotEmitterSettings_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
