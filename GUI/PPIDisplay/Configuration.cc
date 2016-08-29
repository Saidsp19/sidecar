#include "GUI/ChannelImaging.h"
#include "GUI/ChannelSetting.h"
#include "GUI/CLUTSetting.h"
#include "GUI/ColorButtonSetting.h"
#include "GUI/OpacitySetting.h"
#include "GUI/PathSetting.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/BugPlotEmitterSettings.h"
#include "GUI/QCheckBoxSetting.h"
#include "GUI/QComboBoxSetting.h"
#include "GUI/QDoubleSpinBoxSetting.h"
#include "GUI/QGroupBoxSetting.h"
#include "GUI/QLineEditSetting.h"
#include "GUI/QRadioButtonSetting.h"
#include "GUI/QSliderSetting.h"
#include "GUI/QSpinBoxSetting.h"
#include "GUI/QTabWidgetSetting.h"
#include "GUI/RangeRingsImaging.h"
#include "GUI/RangeTruthsImaging.h"
#include "GUI/SampleImaging.h"
#include "GUI/TargetPlotImaging.h"
#include "GUI/VertexColorArray.h"
#include "Messages/BinaryVideo.h"
#include "Messages/BugPlot.h"
#include "Messages/Extraction.h"
#include "Messages/RadarConfig.h"
#include "Messages/TSPI.h"
#include "Messages/Video.h"
#include "Utils/SineCosineLUT.h"

#include "App.h"
#include "BackgroundImageSettings.h"
#include "ChannelSelectorWindow.h"
#include "Configuration.h"
#include "ConfigurationWindow.h"
#include "ControlsWindow.h"
#include "DecaySettings.h"
#include "HistorySettings.h"
#include "PlotPositionFunctor.h"
#include "VideoImaging.h"
#include "VideoSampleCountTransform.h"
#include "ViewSettings.h"

using namespace SideCar::Messages;
using namespace SideCar::GUI::PPIDisplay;

Configuration::Configuration(ChannelSelectorWindow* win1,
                             ConfigurationWindow* win2,
                             ControlsWindow* win3)
    : PresetManager("Configuration")
{
    App* app = App::GetApp();
    BoolSetting* setting;

    setting = new QRadioButtonSetting(this, win2->distanceUnitsKm_, true);
    if (setting->getValue())
	app->setDistanceUnits("km");

    setting = new QRadioButtonSetting(this, win2->distanceUnitsNm_, true);
    if (setting->getValue())
	app->setDistanceUnits("nm");

    setting = new QRadioButtonSetting(this, win2->angleFormatMinSec_, true);
    if (setting->getValue())
	app->setAngleFormatting(AppBase::kDegreesMinutesSeconds);

    setting = new QRadioButtonSetting(this, win2->angleFormatDecimal_, true);
    if (setting->getValue())
	app->setAngleFormatting(AppBase::kDecimal);

    // ChannelSelectorWindow settings
    //
    videoChannel_ = new ChannelSetting(
	Video::GetMetaTypeInfo().getName(), this, win1->getFoundVideo());
    binaryChannel_ = new ChannelSetting(
	BinaryVideo::GetMetaTypeInfo().getName(), this,
	win1->getFoundBinary());
    extractionsChannel_ = new ChannelSetting(
	Extractions::GetMetaTypeInfo().getName(), this,
	win1->getFoundExtractions());
    rangeTruthsChannel_ = new ChannelSetting(
	TSPI::GetMetaTypeInfo().getName(), this,
	win1->getFoundRangeTruths());
    bugPlotsChannel_ = new ChannelSetting(
	BugPlot::GetMetaTypeInfo().getName(), this,
	win1->getFoundBugPlots());

    // ConfigurationWindow settings (global setting)
    //
    new QTabWidgetSetting(this, win2->tabs_, true);

    gainSetting_ = new QSliderSetting(this, win3->getGainControl());
    cutoffMinSetting_ = new QSliderSetting(this, win3->getCutoffMinControl());
    cutoffMaxSetting_ = new QSliderSetting(this, win3->getCutoffMaxControl());

    videoSampleCountTransform_ = new VideoSampleCountTransform(
	new QSpinBoxSetting(this, win2->sampleMin_),
	new QSpinBoxSetting(this, win2->sampleMax_),
	gainSetting_, cutoffMinSetting_, cutoffMaxSetting_,
	new QCheckBoxSetting(this, win2->videoShowDecibels_));

    viewSettings_ = new ViewSettings(
	new DoubleSetting(this, "rangeMaxMax", 400.0),
	new QDoubleSpinBoxSetting(this, win2->rangeMax_),
	new QDoubleSpinBoxSetting(this, win2->viewX_),
	new QDoubleSpinBoxSetting(this, win2->viewY_),
	new QSpinBoxSetting(this, win2->viewZoomPower_),
	new QDoubleSpinBoxSetting(this, win2->viewZoomFactor_));

    new QRadioButtonSetting(this, win2->distanceUnitsKm_, true);
    new QRadioButtonSetting(this, win2->distanceUnitsNm_, true);
    new QRadioButtonSetting(this, win2->angleFormatMinSec_, true);
    new QRadioButtonSetting(this, win2->angleFormatDecimal_, true);

    decaySettings_ = new DecaySettings(
	new QGroupBoxSetting(this, win2->decayEnabled_),
	new QComboBoxSetting(this, win2->decayProfile_),
	new QSpinBoxSetting(this, win2->decayDistance_));

    historySettings_ = new HistorySettings(
	new QGroupBoxSetting(this, win2->historyEnabled_, true),
	new QSpinBoxSetting(this, win2->historyRetention_, true));

    videoImaging_ = new VideoImaging(
	new QGroupBoxSetting(this, win2->videoVisible_),
	new ColorButtonSetting(this, win2->videoColor_),
	new QDoubleSpinBoxSetting(this, win2->videoPointSize_),
	new OpacitySetting(this, win2->videoOpacity_),
	new QComboBoxSetting(this, win2->videoDecimation_),
	new QGroupBoxSetting(this, win2->videoColorMapEnabled_),
	new CLUTSetting(this, win2->videoColorMap_),
	new QGroupBoxSetting(this, win2->videoDesaturateEnabled_),
	new QSliderSetting(this, win2->videoDesaturateRate_));

    binaryImaging_ = new SampleImaging(
	new QGroupBoxSetting(this, win2->binaryVisible_),
	new ColorButtonSetting(this, win2->binaryColor_),
	new QDoubleSpinBoxSetting(this, win2->binaryPointSize_),
	new OpacitySetting(this, win2->binaryOpacity_),
	new QComboBoxSetting(this, win2->binaryDecimation_));

    plotPositionFunctor_ = new PlotPositionFunctor;

    extractionsImaging_ = new TargetPlotImaging(
	plotPositionFunctor_,
	new QGroupBoxSetting(this, win2->extractionsVisible_), 
	new ColorButtonSetting(this, win2->extractionsColor_),
	new QDoubleSpinBoxSetting(this, win2->extractionsExtent_),
	new OpacitySetting(this, win2->extractionsOpacity_),
	new QComboBoxSetting(this, win2->extractionsSymbolType_),
	new QDoubleSpinBoxSetting(this, win2->extractionsLineWidth_),
	new QSpinBoxSetting(this, win2->extractionsLifeTime_),
	new QCheckBoxSetting(this, win2->extractionsFadeEnabled_),
	0,
	new QSpinBoxSetting(this, win2->extractionsTagSize_),
	new QCheckBoxSetting(this, win2->extractionsShowTags_));

    win2->extractionsSymbolType_->associateWith(extractionsImaging_);

    rangeTruthsImaging_ = new RangeTruthsImaging(
	plotPositionFunctor_,
	new QGroupBoxSetting(this, win2->rangeTruthsVisible_), 
	new ColorButtonSetting(this, win2->rangeTruthsColor_),
	new QDoubleSpinBoxSetting(this, win2->rangeTruthsExtent_),
	new OpacitySetting(this, win2->rangeTruthsOpacity_),
	new QComboBoxSetting(this, win2->rangeTruthsSymbolType_),
	new QDoubleSpinBoxSetting(this, win2->rangeTruthsLineWidth_),
	new QSpinBoxSetting(this, win2->rangeTruthsLifeTime_),
	new QCheckBoxSetting(this, win2->rangeTruthsFadeEnabled_),
	new QCheckBoxSetting(this, win2->rangeTruthsShowTrails_),
	new QSpinBoxSetting(this, win2->rangeTruthsMaxTrailLength_),
	new QSpinBoxSetting(this, win2->rangeTruthsTagSize_),
	new QCheckBoxSetting(this, win2->rangeTruthsShowTags_));

    win2->rangeTruthsSymbolType_->associateWith(rangeTruthsImaging_);

    bugPlotsImaging_ = new TargetPlotImaging(
	plotPositionFunctor_,
	new QGroupBoxSetting(this, win2->bugPlotsVisible_), 
	new ColorButtonSetting(this, win2->bugPlotsColor_),
	new QDoubleSpinBoxSetting(this, win2->bugPlotsExtent_),
	new OpacitySetting(this, win2->bugPlotsOpacity_),
	new QComboBoxSetting(this, win2->bugPlotsSymbolType_),
	new QDoubleSpinBoxSetting(this, win2->bugPlotsLineWidth_),
	new QSpinBoxSetting(this, win2->bugPlotsLifeTime_),
	new QCheckBoxSetting(this, win2->bugPlotsFadeEnabled_),
	0,
	new QSpinBoxSetting(this, win2->bugPlotsTagSize_),
	new QCheckBoxSetting(this, win2->bugPlotsShowTags_));

    win2->bugPlotsSymbolType_->associateWith(bugPlotsImaging_);

    rangeRingsImaging_ = new RangeRingsImaging(
	new QGroupBoxSetting(this, win2->rangeRingsVisible_),
	new ColorButtonSetting(this, win2->rangeRingsColor_),
	new QDoubleSpinBoxSetting(this, win2->rangeRingsLineWidth_),
	new OpacitySetting(this, win2->rangeRingsOpacity_),
	new QSpinBoxSetting(this, win2->rangeRingsAzimuthMajor_),
	new QSpinBoxSetting(this, win2->rangeRingsAzimuthMinor_),
	new QDoubleSpinBoxSetting(this, win2->rangeRingsRangeMajor_),
	new QSpinBoxSetting(this, win2->rangeRingsRangeMinor_));

    rangeMapImaging_ = new ChannelImaging(
	new QGroupBoxSetting(this, win2->rangeMapVisible_),
	new ColorButtonSetting(this, win2->rangeMapColor_),
	new QDoubleSpinBoxSetting(this, win2->rangeMapLineWidth_),
	new OpacitySetting(this, win2->rangeMapOpacity_));

    phantomCursorImaging_ = new PhantomCursorImaging(
	new QGroupBoxSetting(this, win2->phantomCursorVisible_),
	new ColorButtonSetting(this, win2->phantomCursorColor_),
	new QDoubleSpinBoxSetting(this, win2->phantomCursorLineWidth_),
	new OpacitySetting(this, win2->phantomCursorOpacity_),
	new QSpinBoxSetting(this, win2->phantomCursorRadius_));

    backgroundImageSettings_ = new BackgroundImageSettings(
	new QGroupBoxSetting(this, win2->backgroundEnabled_),
	new PathSetting(this, win2->backgroundFile_, win2->backgroundLoad_,
                        "Select Background File",
                        "Images (*.png *.xpm *.jpg)"),
	new OpacitySetting(this, win2->backgroundOpacity_));

    showCursorPositionSetting_ = new OnOffSettingsBlock(
	new BoolSetting(this, "showCursorPosition", true, false));

    showPhantomCursorSetting_ = new OnOffSettingsBlock(
	new BoolSetting(this, "showPhantomCursor", true, false));

    bugPlotEmitterSettings_ = new BugPlotEmitterSettings(
	new QGroupBoxSetting(this, win2->bugPlotEmitterEnabled_),
	new QLineEditSetting(this, win2->bugPlotEmitterServiceName_),
	new QLineEditSetting(this, win2->bugPlotEmitterAddress_),
	bugPlotsChannel_);

    sineCosineLUT_ = Utils::SineCosineLUT::Make(
	RadarConfig::GetShaftEncodingMax() + 1);

    restoreAllPresets();
}
