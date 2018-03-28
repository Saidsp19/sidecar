#include "GUI/BugPlotEmitterSettings.h"
#include "GUI/CLUTSetting.h"
#include "GUI/ChannelImaging.h"
#include "GUI/ChannelSelectorWindow.h"
#include "GUI/ChannelSetting.h"
#include "GUI/ColorButtonSetting.h"
#include "GUI/ControlsWindow.h"
#include "GUI/OpacitySetting.h"
#include "GUI/PathSetting.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/QDoubleSpinBoxSetting.h"
#include "GUI/QGroupBoxSetting.h"
#include "GUI/QLineEditSetting.h"
#include "GUI/QRadioButtonSetting.h"
#include "GUI/QSliderSetting.h"
#include "GUI/QSpinBoxSetting.h"
#include "GUI/QTabWidgetSetting.h"
#include "GUI/RangeTruthsImaging.h"
#include "GUI/SampleImaging.h"
#include "GUI/TargetPlotImaging.h"
#include "GUI/VertexColorArray.h"
#include "GUI/VideoSampleCountTransform.h"
#include "Messages/BugPlot.h"
#include "Messages/Extraction.h"
#include "Messages/TSPI.h"
#include "Messages/Video.h"

#include "App.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "GridImaging.h"
#include "History.h"
#include "RadarSettings.h"
#include "VideoImaging.h"

using namespace SideCar::GUI::ESScope;
using namespace SideCar::Messages;

Configuration::Configuration(ChannelSelectorWindow* win1, ConfigurationWindow* win2, ControlsWindow* win3) :
    PresetManager("Configuration")
{
    App* app = App::GetApp();
    BoolSetting* setting;

    allynHack_ = new QCheckBoxSetting(this, win2->allynHack_);

    setting = new QRadioButtonSetting(this, win2->distanceUnitsKm_, true);
    if (setting->getValue()) app->setDistanceUnits("km");

    setting = new QRadioButtonSetting(this, win2->distanceUnitsNm_, false);
    if (setting->getValue()) app->setDistanceUnits("nm");

    setting = new QRadioButtonSetting(this, win2->angleFormatDecimal_, true);
    if (setting->getValue()) app->setAngleFormatting(AppBase::kDecimal);

    setting = new QRadioButtonSetting(this, win2->angleFormatMinSec_, false);
    if (setting->getValue()) app->setAngleFormatting(AppBase::kDegreesMinutesSeconds);

    // ChannelSelectorWindow settings
    //
    videoChannel_ = new ChannelSetting(Video::GetMetaTypeInfo().getName(), this, win1->getFoundVideo());
    extractionsChannel_ =
        new ChannelSetting(Extractions::GetMetaTypeInfo().getName(), this, win1->getFoundExtractions());
    rangeTruthsChannel_ = new ChannelSetting(TSPI::GetMetaTypeInfo().getName(), this, win1->getFoundRangeTruths());
    bugPlotsChannel_ = new ChannelSetting(BugPlot::GetMetaTypeInfo().getName(), this, win1->getFoundBugPlots());

    // ConfigurationWindow settings (global setting)
    //
    new QTabWidgetSetting(this, win2->tabs_, true);

    gainSetting_ = new QSliderSetting(this, win3->getGainControl());
    cutoffMinSetting_ = new QSliderSetting(this, win3->getCutoffMinControl());
    cutoffMaxSetting_ = new QSliderSetting(this, win3->getCutoffMaxControl());

    videoSampleCountTransform_ = new VideoSampleCountTransform(
        new QSpinBoxSetting(this, win2->sampleMin_), new QSpinBoxSetting(this, win2->sampleMax_), gainSetting_,
        cutoffMinSetting_, cutoffMaxSetting_, new QCheckBoxSetting(this, win2->videoShowDecibels_));

    radarSettings_ = new RadarSettings(
        new QSpinBoxSetting(this, win2->alphaScans_), new QSpinBoxSetting(this, win2->betaScans_),
        new QSpinBoxSetting(this, win2->rangeScans_), new QDoubleSpinBoxSetting(this, win2->alphaMin_),
        new QDoubleSpinBoxSetting(this, win2->alphaMax_), new QDoubleSpinBoxSetting(this, win2->betaMin_),
        new QDoubleSpinBoxSetting(this, win2->betaMax_), new QDoubleSpinBoxSetting(this, win2->rangeMin_),
        new QDoubleSpinBoxSetting(this, win2->rangeMax_), new QDoubleSpinBoxSetting(this, win2->sampleRangeMin_),
        new QDoubleSpinBoxSetting(this, win2->sampleRangeFactor_), new QDoubleSpinBoxSetting(this, win2->faceTilt_),
        new QDoubleSpinBoxSetting(this, win2->faceRotation_), new QSpinBoxSetting(this, win2->firstSample_),
        new QSpinBoxSetting(this, win2->lastSample_), allynHack_,
        new QCheckBoxSetting(this, win2->ignoreMessageRangeSettings_));

    videoImaging_ = new VideoImaging(
        new QGroupBoxSetting(this, win2->videoVisible_), new ColorButtonSetting(this, win2->videoColor_),
        new QDoubleSpinBoxSetting(this, win2->videoPointSize_), new OpacitySetting(this, win2->videoOpacity_),
        new QComboBoxSetting(this, win2->videoDecimation_), new QCheckBoxSetting(this, win2->videoColorMapEnabled_),
        new CLUTSetting(this, win2->videoColorMap_));
    win2->videoPointSize_->setEnabled(true);

    extractionsImaging_ = new TargetPlotImaging(
        0, new QGroupBoxSetting(this, win2->extractionsVisible_), new ColorButtonSetting(this, win2->extractionsColor_),
        new QDoubleSpinBoxSetting(this, win2->extractionsExtent_), new OpacitySetting(this, win2->extractionsOpacity_),
        new QComboBoxSetting(this, win2->extractionsSymbolType_),
        new QDoubleSpinBoxSetting(this, win2->extractionsLineWidth_),
        new QSpinBoxSetting(this, win2->extractionsLifeTime_),
        new QCheckBoxSetting(this, win2->extractionsFadeEnabled_), 0,
        new QSpinBoxSetting(this, win2->extractionsTagSize_), new QCheckBoxSetting(this, win2->extractionsShowTags_));

    win2->extractionsSymbolType_->associateWith(extractionsImaging_);

    rangeTruthsImaging_ = new RangeTruthsImaging(
        0, new QGroupBoxSetting(this, win2->rangeTruthsVisible_), new ColorButtonSetting(this, win2->rangeTruthsColor_),
        new QDoubleSpinBoxSetting(this, win2->rangeTruthsExtent_), new OpacitySetting(this, win2->rangeTruthsOpacity_),
        new QComboBoxSetting(this, win2->rangeTruthsSymbolType_),
        new QDoubleSpinBoxSetting(this, win2->rangeTruthsLineWidth_),
        new QSpinBoxSetting(this, win2->rangeTruthsLifeTime_),
        new QCheckBoxSetting(this, win2->rangeTruthsFadeEnabled_),
        new QCheckBoxSetting(this, win2->rangeTruthsShowTrails_),
        new QSpinBoxSetting(this, win2->rangeTruthsMaxTrailLength_),
        new QSpinBoxSetting(this, win2->rangeTruthsTagSize_), new QCheckBoxSetting(this, win2->rangeTruthsShowTags_));

    win2->rangeTruthsSymbolType_->associateWith(rangeTruthsImaging_);

    bugPlotsImaging_ = new TargetPlotImaging(
        0, new QGroupBoxSetting(this, win2->bugPlotsVisible_), new ColorButtonSetting(this, win2->bugPlotsColor_),
        new QDoubleSpinBoxSetting(this, win2->bugPlotsExtent_), new OpacitySetting(this, win2->bugPlotsOpacity_),
        new QComboBoxSetting(this, win2->bugPlotsSymbolType_),
        new QDoubleSpinBoxSetting(this, win2->bugPlotsLineWidth_), new QSpinBoxSetting(this, win2->bugPlotsLifeTime_),
        new QCheckBoxSetting(this, win2->bugPlotsFadeEnabled_), 0, new QSpinBoxSetting(this, win2->bugPlotsTagSize_),
        new QCheckBoxSetting(this, win2->bugPlotsShowTags_));

    win2->bugPlotsSymbolType_->associateWith(bugPlotsImaging_);

    history_ =
        new History(this, radarSettings_, extractionsImaging_, rangeTruthsImaging_, bugPlotsImaging_, allynHack_);
    connect(videoChannel_, SIGNAL(incoming(const MessageList&)), history_, SLOT(processVideo(const MessageList&)));
    connect(videoChannel_, SIGNAL(valueChanged(int)), history_, SLOT(clearVideo()));

    connect(extractionsChannel_, SIGNAL(incoming(const MessageList&)), history_,
            SLOT(processExtractions(const MessageList&)));
    connect(extractionsChannel_, SIGNAL(valueChanged(int)), history_, SLOT(clearExtractions()));

    connect(rangeTruthsChannel_, SIGNAL(incoming(const MessageList&)), history_,
            SLOT(processRangeTruths(const MessageList&)));
    connect(rangeTruthsChannel_, SIGNAL(valueChanged(int)), history_, SLOT(clearRangeTruths()));

    connect(bugPlotsChannel_, SIGNAL(incoming(const MessageList&)), history_,
            SLOT(processBugPlots(const MessageList&)));
    connect(bugPlotsChannel_, SIGNAL(valueChanged(int)), history_, SLOT(clearBugPlots()));

    gridImaging_ = new GridImaging(
        new QGroupBoxSetting(this, win2->gridVisible_), new ColorButtonSetting(this, win2->gridColor_),
        new QDoubleSpinBoxSetting(this, win2->gridLineWidth_), new OpacitySetting(this, win2->gridOpacity_),
        new QSpinBoxSetting(this, win2->gridAlphaMajor_), new QSpinBoxSetting(this, win2->gridAlphaMinor_),
        new QSpinBoxSetting(this, win2->gridBetaMajor_), new QSpinBoxSetting(this, win2->gridBetaMinor_),
        new QSpinBoxSetting(this, win2->gridRangeMajor_), new QSpinBoxSetting(this, win2->gridRangeMinor_));

    phantomCursorImaging_ = new PhantomCursorImaging(new QGroupBoxSetting(this, win2->phantomCursorVisible_),
                                                     new ColorButtonSetting(this, win2->phantomCursorColor_),
                                                     new QDoubleSpinBoxSetting(this, win2->phantomCursorLineWidth_),
                                                     new OpacitySetting(this, win2->phantomCursorOpacity_),
                                                     new QSpinBoxSetting(this, win2->phantomCursorRadius_));

    showPhantomCursorSetting_ = new OnOffSettingsBlock(new BoolSetting(this, "showPhantomCursor", true, false));

    bugPlotEmitterSettings_ =
        new BugPlotEmitterSettings(new QGroupBoxSetting(this, win2->bugPlotEmitterEnabled_),
                                   new QLineEditSetting(this, win2->bugPlotEmitterServiceName_),
                                   new QLineEditSetting(this, win2->bugPlotEmitterAddress_), bugPlotsChannel_);

    restoreAllPresets();
}
