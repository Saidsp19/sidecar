#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QGridLayout"
#include "QtGui/QIcon"
#include "QtGui/QMenu"
#include "QtGui/QMessageBox"
#include "QtGui/QPushButton"
#include "QtGui/QStatusBar"
#include "QtGui/QToolButton"

#include "GUI/ChannelInfoWidget.h"
#include "GUI/ChannelMenu.h"
#include "GUI/ColorMapWidget.h"
#include "GUI/ControlsWidget.h"
#include "GUI/LogUtils.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/PresetChooser.h"
#include "GUI/PresetsWindow.h"
#include "GUI/QSliderSetting.h"
#include "GUI/RadarInfoWidget.h"
#include "GUI/RangeRingsImaging.h"
#include "GUI/SvgIconMaker.h"
#include "GUI/TargetPlotSymbolsWidget.h"
#include "GUI/ToolBar.h"
#include "GUI/Utils.h"
#include "GUI/WindowManager.h"
#include "Utils/Utils.h"

#include "App.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "FramesWindow.h"
#include "HistorySettings.h"
#include "MagnifierWindow.h"
#include "MainWindow.h"
#include "PastImage.h"
#include "PlayerWindow.h"
#include "PPIWidget.h"
#include "ScaleWidget.h"
#include "TargetPlotImaging.h"
#include "VideoImaging.h"
#include "ViewSettings.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::BScope;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("bscope.MainWindow");
    return log_;
}

MainWindow::MainWindow()
    : MainWindowBase(), Ui::MainWindow(), display_(0)
{
    setupUi(this);
    setObjectName("MainWindow");
#ifdef __DEBUG__
    setWindowTitle("B-Scope (DEBUG)");
#else
    setWindowTitle("B-Scope");
#endif

    // Create the live widget. This has a PPIWidget and two ScaleWidgets.
    //
    QGridLayout* layout = new QGridLayout(live_);
    layout->setSpacing(0);
    layout->setMargin(0);

    rangeScale_ = new ScaleWidget(live_, Qt::Vertical);
    layout->addWidget(rangeScale_, 0, 0);

    display_ = new PPIWidget(live_);
    layout->addWidget(display_, 0, 1);

    azimuthScale_ = new DegreesScaleWidget(live_, Qt::Horizontal);
    azimuthScale_->setMajorTickDivisions(4);
    layout->addWidget(azimuthScale_, 1, 1);

    App* app = getApp();
    connect(app, SIGNAL(phantomCursorChanged(const QPointF&)),
            SLOT(setPhantomCursor(const QPointF&)));

    Configuration* cfg = app->getConfiguration();
    viewSettings_ = cfg->getViewSettings();

    updatePresetActions();
    connect(cfg, SIGNAL(presetDirtyStateChanged(int, bool)),
            SLOT(updatePresetActions()));
    connect(cfg, SIGNAL(activePresetChanged(int)),
            SLOT(updatePresetActions()));

    // Associate QAction objects with configuration settings.
    //
    cfg->getVideoImaging()->setToggleEnabledAction(actionViewToggleVideo_);
    cfg->getBinaryImaging()->setToggleEnabledAction(actionViewToggleBinary_);
    cfg->getExtractionsImaging()->setToggleEnabledAction(
	actionViewToggleExtractions_);
    cfg->getRangeTruthsImaging()->setToggleEnabledAction(
	actionViewToggleRangeTruths_);
    cfg->getBugPlotsImaging()->setToggleEnabledAction(
	actionViewToggleBugPlots_);
    cfg->getRangeMapImaging()->setToggleEnabledAction(
	actionViewToggleRangeMap_);
    cfg->getRangeRingsImaging()->setToggleEnabledAction(
	actionViewToggleRangeRings_);
    cfg->getShowCursorPositionSetting()->setToggleEnabledAction(
	actionViewToggleShowCursorPosition_);
    cfg->getShowPhantomCursorSetting()->setToggleEnabledAction(
	actionViewTogglePhantomCursor_);

    connect(cfg->getPhantomCursorImaging(), SIGNAL(enabledChanged(bool)),
            actionViewTogglePhantomCursor_, SLOT(setEnabled(bool)));
    actionViewTogglePhantomCursor_->setEnabled(
	cfg->getPhantomCursorImaging()->isEnabled());

    connect(cfg->getShowCursorPositionSetting(),
            SIGNAL(enabledChanged(bool)), display_,
            SLOT(setShowCursorPosition(bool)));
    display_->setShowCursorPosition(
	cfg->getShowCursorPositionSetting()->isEnabled());

    SvgIconMaker im;
    actionViewToggleVideo_->setIcon(im.make('V'));
    actionViewToggleBinary_->setIcon(im.make('B'));
    actionViewToggleExtractions_->setIcon(im.make('E'));
    actionViewToggleRangeTruths_->setIcon(im.make('T'));
    actionViewToggleBugPlots_->setIcon(im.make('P'));
    actionViewToggleRangeMap_->setIcon(im.make('M'));
    actionViewToggleRangeRings_->setIcon(im.make('R'));
    actionViewToggleShowCursorPosition_->setIcon(im.make("cursorPosition"));
    actionViewTogglePhantomCursor_->setIcon(im.make("phantomCursor"));

    ToolBar* toolBar;
    QAction* action;

    // Cursor info widget
    //
    CursorWidget* cursorWidget = new CursorWidget(statusBar());
    statusBar()->addPermanentWidget(cursorWidget);

    // Radar info widget
    //
    RadarInfoWidget* radarInfoWidget = new RadarInfoWidget(statusBar());
    connect(display_,
            SIGNAL(currentMessage(const Messages::PRIMessage::Ref&)),
            radarInfoWidget,
            SLOT(showMessageInfo(const Messages::PRIMessage::Ref&)));
    statusBar()->addPermanentWidget(radarInfoWidget);

    // Channel connection info widget
    //
    ChannelInfoWidget* channelInfoWidget = new ChannelInfoWidget(statusBar());
    statusBar()->addPermanentWidget(channelInfoWidget);
    channelInfoWidget->addChannel("V:", cfg->getVideoChannel());
    channelInfoWidget->addChannel("B:", cfg->getBinaryChannel());
    channelInfoWidget->addChannel("E:", cfg->getExtractionsChannel());
    channelInfoWidget->addChannel("T:", cfg->getRangeTruthsChannel());
    channelInfoWidget->addChannel("P:", cfg->getBugPlotsChannel());

    // Create a toolbar for the QAction objects that toggle tool window.
    //
    toolBar = makeToolBar("Tool Windows", Qt::LeftToolBarArea);

    action = app->getToolsMenuAction(App::kShowChannelSelectorWindow);
    action->setIcon(QIcon(":/channelsWindow.png"));
    toolBar->addAction(action);

    action = app->getToolsMenuAction(App::kShowConfigurationWindow);
    action->setIcon(QIcon(":/configurationWindow.png"));
    toolBar->addAction(action);

    action = app->getToolsMenuAction(App::kShowControlsWindow);
    action->setIcon(QIcon(":/controlsWindow.png"));
    toolBar->addAction(action);

    // Create a toolbar for the QAction objects that toggle data display
    //
    toolBar = makeToolBar("Layer Controls", Qt::LeftToolBarArea);
    toolBar->addAction(actionViewToggleVideo_);
    QToolButton* toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleVideo_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(cfg->getVideoChannel(), toolButton));

    toolBar->addAction(actionViewToggleBinary_);
    toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleBinary_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(cfg->getBinaryChannel(), toolButton));

    toolBar->addAction(actionViewToggleExtractions_);
    toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleExtractions_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(cfg->getExtractionsChannel(), toolButton));

    toolBar->addAction(actionViewToggleRangeTruths_);
    toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleRangeTruths_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(cfg->getRangeTruthsChannel(), toolButton));

    toolBar->addAction(actionViewToggleBugPlots_);
    toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleBugPlots_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(cfg->getBugPlotsChannel(), toolButton));

    toolBar->addAction(actionViewToggleRangeMap_);
    toolBar->addAction(actionViewToggleRangeRings_);
    toolBar->addAction(actionViewToggleShowCursorPosition_);
    toolBar->addAction(actionViewTogglePhantomCursor_);
    app->addWindowMenuActionTo(toolBar, WindowManager::kFullScreen);

    // Create a toolbar for setting presets
    //
    toolBar = makeToolBar("Presets", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    action = app->getToolsMenuAction(App::kShowPresetsWindow);
    action->setIcon(QIcon(":/viewEditor.svg"));
    toolBar->addAction(action);

    PresetChooser* presetChooser = new PresetChooser(cfg, toolBar);
    toolBar->addWidget(presetChooser);
    connect(cfg, SIGNAL(presetDirtyStateChanged(int, bool)),
            SLOT(monitorPresets(int, bool)));

    // Create a toolbar for the colorbar
    //
    toolBar = makeToolBar("ColorMap", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    ColorMapWidget* colorMapWidget =
	new ColorMapWidget(cfg->getVideoSampleCountTransform(), toolBar);
	     
    toolBar->addWidget(colorMapWidget);
    VideoImaging* videoImaging = cfg->getVideoImaging();
    colorMapWidget->setColorMap(videoImaging->getColorMap());
    connect(videoImaging, SIGNAL(colorMapChanged(const QImage&)),
            colorMapWidget, SLOT(setColorMap(const QImage&)));
    connect(colorMapWidget, SIGNAL(changeColorMapType(int)),
            videoImaging, SLOT(setColorMapType(int)));

    // Create a toolbar to show the plot symbol assignments.
    //
    toolBar = makeToolBar("Plot Symbols", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    TargetPlotSymbolsWidget* symbolsWidget =
	new TargetPlotSymbolsWidget(toolBar);
    toolBar->addWidget(symbolsWidget);
    symbolsWidget->connectExtractionsSymbolType(cfg->getExtractionsImaging());
    symbolsWidget->connectRangeTruthsSymbolType(cfg->getRangeTruthsImaging());
    symbolsWidget->connectBugPlotsSymbolType(cfg->getBugPlotsImaging());

    // Create a toolbar for the gain and min/max controls
    //
    toolBar = makeToolBar("Gain and Min/Max", Qt::RightToolBarArea);
    ControlsWidget* controlsWidget = new ControlsWidget(toolBar);
    controlsWidget->addControl("Gain", cfg->getGainSetting());
    controlsWidget->addControl("Min", cfg->getCutoffMinSetting());
    controlsWidget->addControl("Max", cfg->getCutoffMaxSetting());
    toolBar->addWidget(controlsWidget);

    connect(cfg->getRangeRingsImaging(), SIGNAL(settingChanged()),
            SLOT(rangeRingsImagingChanged()));
    rangeRingsImagingChanged();

    connect(display_, SIGNAL(transformChanged()), this,
            SLOT(displayTransformChanged()));
    displayTransformChanged();

    connect(cfg->getShowPhantomCursorSetting(),
            SIGNAL(enabledChanged(bool)), display_,
            SLOT(showPhantomCursor(bool)));
    display_->showPhantomCursor(
	cfg->getShowPhantomCursorSetting()->isEnabled());
}

void
MainWindow::initialize()
{
    Super::initialize();
}

void
MainWindow::on_actionViewClearAll__triggered()
{
    display_->clearAll();
}

void
MainWindow::on_actionViewClearVideo__triggered()
{
    display_->clearVideoBuffer();
}

void
MainWindow::on_actionViewClearBinaryVideo__triggered()
{
    display_->clearBinaryBuffer();
}

void
MainWindow::on_actionViewClearExtractions__triggered()
{
    display_->clearExtractions();
}

void
MainWindow::on_actionViewClearRangeTruths__triggered()
{
    display_->clearRangeTruths();
}

void
MainWindow::on_actionViewClearBugPlots__triggered()
{
    display_->clearBugPlots();
}

void
MainWindow::displayTransformChanged()
{
    Logger::ProcLog log("displayTransformChanged", Log());
    QSize displaySize = display_->size();

    getApp()->setImageSize(displaySize);

    rangeScale_->setStartAndEnd(viewSettings_->getRangeMin(),
                                viewSettings_->getRangeMax());
    azimuthScale_->setStartAndEnd(viewSettings_->getAzimuthMin(),
                                  viewSettings_->getAzimuthMax());

    azimuthConversion_ = double(azimuthScale_->getSpan()) /
	viewSettings_->getAzimuthSpan();
    rangeConversion_ = double(rangeScale_->getSpan()) /
	viewSettings_->getRangeSpan();

    rangeRingsImagingChanged();
}

void
MainWindow::setPhantomCursor(const QPointF& pos)
{
    azimuthScale_->setNormalizedCursorPosition(pos.x());
    rangeScale_->setNormalizedCursorPosition(pos.y());
}

void
MainWindow::rangeRingsImagingChanged()
{
    RangeRingsImaging* imaging =
	getApp()->getConfiguration()->getRangeRingsImaging();

    int rangeTicks = int(::rint(viewSettings_->getRangeSpan() /
                                imaging->getRangeSpacing()));
    rangeScale_->setMajorTickDivisions(rangeTicks);
    rangeScale_->setMinorTickDivisions(imaging->getRangeTicks());

    int azimuthTicks = int(::rint(viewSettings_->getAzimuthSpan() /
                                  Utils::degreesToRadians(
                                      imaging->getAzimuthSpacing())));
    azimuthScale_->setMajorTickDivisions(azimuthTicks);
    azimuthScale_->setMinorTickDivisions(imaging->getAzimuthTicks());
}

void
MainWindow::on_actionShowPlayerWindow__triggered()
{
    getApp()->getPlayerWindow()->showAndRaise();
}

void
MainWindow::on_actionShowFramesWindow__triggered()
{
    getApp()->getFramesWindow()->showAndRaise();
}

void
MainWindow::on_actionPresetRevert__triggered()
{
    Configuration* configuration = getApp()->getConfiguration();
    configuration->restorePreset(configuration->getActivePresetIndex());
    statusBar()->showMessage("Restored current preset.", 5000);
}

void
MainWindow::on_actionPresetSave__triggered()
{
    Configuration* configuration = getApp()->getConfiguration();
    configuration->savePreset(configuration->getActivePresetIndex());
    statusBar()->showMessage("Saved current preset.", 5000);
}

void
MainWindow::updatePresetActions()
{
    Configuration* configuration = getApp()->getConfiguration();
    bool dirty = configuration->getActiveIsDirty();
    actionPresetSave_->setEnabled(dirty);
    actionPresetRevert_->setEnabled(dirty);
}

void
MainWindow::showEvent(QShowEvent*  event)
{
    Super::showEvent(event);
    QTimer::singleShot(0, display_, SLOT(raiseMagnifiers()));
    display_->setFocus();
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    // Check if there are any unsaved preferences, and alert the user. If the quit is aborted, show the presets
    // window so that the user can manage the unsaved presets.
    //
    App* app = getApp();
    if (! app->isQuitting()) {
	event->ignore();
	QTimer::singleShot(0, app, SLOT(applicationQuit()));
	return;
    }

    Super::closeEvent(event);
}

void
MainWindow::saveToSettings(QSettings& settings)
{
    Super::saveToSettings(settings);
    settings.setValue("showPhantomCursor", display_->getShowPhantomCursor());
    display_->saveMagnifiers(settings);
}

void
MainWindow::restoreFromSettings(QSettings& settings)
{
    display_->setSettingsKey(settings.group());
    Super::restoreFromSettings(settings);
}

void
MainWindow::monitorPresets(int index, bool isDirty)
{
    if (! isDirty)
	statusBar()->showMessage("Preset saved.", 5000);
    else
	statusBar()->showMessage("Preset changed.", 5000);
}
