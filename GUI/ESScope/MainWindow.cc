#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QHBoxLayout"
#include "QtGui/QIcon"
#include "QtGui/QMenu"
#include "QtGui/QMessageBox"
#include "QtGui/QPushButton"
#include "QtGui/QStatusBar"
#include "QtGui/QToolButton"

#include "GUI/BugPlotEmitterSettings.h"
#include "GUI/ChannelInfoWidget.h"
#include "GUI/ChannelMenu.h"
#include "GUI/ColorMapWidget.h"
#include "GUI/ControlsWidget.h"
#include "GUI/LogUtils.h"
#include "GUI/PhantomCursorImaging.h"
#include "GUI/PresetChooser.h"
#include "GUI/PresetsWindow.h"
#include "GUI/QSliderSetting.h"
#include "GUI/SvgIconMaker.h"
#include "GUI/RangeTruthsImaging.h"
#include "GUI/TargetPlotImaging.h"
#include "GUI/TargetPlotSymbolsWidget.h"
#include "GUI/ToolBar.h"
#include "GUI/Utils.h"
#include "GUI/WindowManager.h"
#include "Utils/Utils.h"

#include "AlphaBetaView.h"
#include "AlphaBetaViewSettings.h"
#include "AlphaBetaWidget.h"
#include "AlphaRangeView.h"
#include "AlphaRangeViewSettings.h"
#include "AlphaRangeWidget.h"
#include "App.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "GridImaging.h"
#include "History.h"
#include "MainWindow.h"
#include "RadarInfoWidget.h"
#include "RadarSettings.h"
#include "VideoImaging.h"
#include "ViewChooser.h"
#include "ViewEditor.h"
#include "XYWidget.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::ESScope;

static int kUpdateRate = 33;	// msecs between updateGL() calls (~30 FPS)

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("esscope.MainWindow");
    return log_;
}

MainWindow::MainWindow()
    : MainWindowBase(), Ui::MainWindow(), alphaBetaView_(0),
      alphaBetaViewSettings_(0), alphaRangeView_(0),
      alphaRangeViewSettings_(0), updateTimer_(), configuration_(0)
{
    setupUi(this);
    setObjectName("MainWindow");
#ifdef __DEBUG__
    setWindowTitle("ES Display (DEBUG)");
#else
    setWindowTitle("ES Display");
#endif

    App* app = getApp();
    configuration_ = app->getConfiguration();
    History* history = configuration_->getHistory();
    QHBoxLayout* layout = new QHBoxLayout(centralWidget());

    alphaBetaViewSettings_ = new AlphaBetaViewSettings(
	this, configuration_->getRadarSettings());
    connect(alphaBetaViewSettings_, SIGNAL(viewChanged()),
            SLOT(updateAlphaBetaViewActions()));

    alphaBetaView_ = new AlphaBetaView(this, alphaBetaViewSettings_);
    layout->addWidget(alphaBetaView_);

    alphaRangeViewSettings_ = new AlphaRangeViewSettings(
	this, configuration_->getRadarSettings());
    connect(alphaRangeViewSettings_, SIGNAL(viewChanged()),
            SLOT(updateAlphaRangeViewActions()));

    alphaRangeView_ = new AlphaRangeView(this, alphaRangeViewSettings_);
    layout->addWidget(alphaRangeView_);

    app->getViewEditor()->setViewSettings(alphaBetaViewSettings_,
                                          alphaRangeViewSettings_);

    centralWidget()->setLayout(layout);

    updatePresetActions();
    connect(configuration_, SIGNAL(presetDirtyStateChanged(int, bool)),
            SLOT(updatePresetActions()));
    connect(configuration_, SIGNAL(activePresetChanged(int)),
            SLOT(updatePresetActions()));

    // Associate QAction objects with configuration settings.
    //
    configuration_->getVideoImaging()->setToggleEnabledAction(
	actionViewToggleVideo_);
    configuration_->getExtractionsImaging()->setToggleEnabledAction(
	actionViewToggleExtractions_);
    configuration_->getRangeTruthsImaging()->setToggleEnabledAction(
	actionViewToggleRangeTruths_);
    configuration_->getBugPlotsImaging()->setToggleEnabledAction(
	actionViewToggleBugPlots_);
    configuration_->getGridImaging()->setToggleEnabledAction(
	actionViewToggleGrid_);
    configuration_->getShowPhantomCursorSetting()->setToggleEnabledAction(
	actionViewTogglePhantomCursor_);

    connect(configuration_->getPhantomCursorImaging(),
            SIGNAL(enabledChanged(bool)),
            actionViewTogglePhantomCursor_, SLOT(setEnabled(bool)));
    actionViewTogglePhantomCursor_->setEnabled(
	configuration_->getPhantomCursorImaging()->isEnabled());

    SvgIconMaker im;
    actionViewToggleVideo_->setIcon(im.make('V'));
    actionViewToggleExtractions_->setIcon(im.make('E'));
    actionViewToggleRangeTruths_->setIcon(im.make('T'));
    actionViewToggleBugPlots_->setIcon(im.make('P'));
    actionViewToggleGrid_->setIcon(im.make('G'));
    actionViewTogglePhantomCursor_->setIcon(im.make("phantomCursor"));

    ToolBar* toolBar;
    QAction* action;

    // Cursor info widget
    //
    CursorWidget* cursorWidget = new CursorWidget(statusBar());
    statusBar()->addPermanentWidget(cursorWidget);

    connect(alphaBetaView_->getDisplay(),
            SIGNAL(cursorMoved(double, double)),
            cursorWidget, SLOT(setAlphaBeta(double, double)));

    connect(alphaBetaView_->getDisplay(), SIGNAL(bugged()),
            SLOT(bugged()));

    connect(alphaBetaView_->getDisplay(),
            SIGNAL(cursorMoved(double, double)),
            alphaRangeView_->getDisplay(), SLOT(setSlaveAlpha(double)));

    connect(alphaRangeView_->getDisplay(),
            SIGNAL(cursorMoved(double, double)),
            cursorWidget, SLOT(setRange(double, double)));

    connect(alphaRangeView_->getDisplay(), SIGNAL(bugged()),
            SLOT(bugged()));

    connect(alphaRangeView_->getDisplay(),
            SIGNAL(cursorMoved(double, double)),
            alphaBetaView_->getDisplay(), SLOT(setSlaveAlpha(double)));

    // Radar info widget
    //
    RadarInfoWidget* radarInfoWidget = new RadarInfoWidget(statusBar());
    connect(history,
            SIGNAL(currentMessage(const Messages::PRIMessage::Ref&)),
            radarInfoWidget,
            SLOT(showMessageInfo(const Messages::PRIMessage::Ref&)));
    statusBar()->addPermanentWidget(radarInfoWidget);

    // Channel connection info widget
    //
    ChannelInfoWidget* channelInfoWidget = new ChannelInfoWidget(statusBar());
    statusBar()->addPermanentWidget(channelInfoWidget);
    channelInfoWidget->addChannel(
	"V:", configuration_->getVideoChannel());
    channelInfoWidget->addChannel(
	"E:", configuration_->getExtractionsChannel());
    channelInfoWidget->addChannel(
	"T:", configuration_->getRangeTruthsChannel());
    channelInfoWidget->addChannel(
	"P:", configuration_->getBugPlotsChannel());

    actionAlphaBetaViewFull_->setIcon(QIcon(":/home.png"));
    actionAlphaBetaViewFull_->setEnabled(false);
    actionAlphaBetaViewPrevious_->setIcon(QIcon(":/popViewStack.png"));
    actionAlphaBetaViewPrevious_->setEnabled(false);
    actionAlphaBetaViewSwap_->setIcon(QIcon(":/refresh.png"));
    actionAlphaBetaViewSwap_->setEnabled(false);

    toolBar = makeToolBar("Alpha/Beta View", Qt::LeftToolBarArea);
    toolBar->setAllowedAreas(Qt::LeftToolBarArea);
    toolBar->addAction(actionAlphaBetaViewFull_);
    toolBar->addAction(actionAlphaBetaViewPrevious_);
    toolBar->addAction(actionAlphaBetaViewSwap_);

    actionAlphaRangeViewFull_->setIcon(QIcon(":/home.png"));
    actionAlphaRangeViewFull_->setEnabled(false);
    actionAlphaRangeViewPrevious_->setIcon(QIcon(":/popViewStack.png"));
    actionAlphaRangeViewPrevious_->setEnabled(false);
    actionAlphaRangeViewSwap_->setIcon(QIcon(":/refresh.png"));
    actionAlphaRangeViewSwap_->setEnabled(false);

    toolBar = makeToolBar("Alpha/Range View", Qt::RightToolBarArea);
    toolBar->setAllowedAreas(Qt::RightToolBarArea);
    toolBar->addAction(actionAlphaRangeViewFull_);
    toolBar->addAction(actionAlphaRangeViewPrevious_);
    toolBar->addAction(actionAlphaRangeViewSwap_);

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
	new ChannelMenu(configuration_->getVideoChannel(), toolButton));
    toolBar->addAction(actionViewToggleExtractions_);
    toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleExtractions_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(configuration_->getExtractionsChannel(),
                        toolButton));
    toolBar->addAction(actionViewToggleRangeTruths_);
    toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleRangeTruths_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(configuration_->getRangeTruthsChannel(),
                        toolButton));
    toolBar->addAction(actionViewToggleBugPlots_);
    toolButton = qobject_cast<QToolButton*>(
	toolBar->widgetForAction(actionViewToggleBugPlots_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(
	new ChannelMenu(configuration_->getBugPlotsChannel(), toolButton));
    toolBar->addAction(actionViewToggleGrid_);
    toolBar->addAction(actionViewTogglePhantomCursor_);
    app->addWindowMenuActionTo(toolBar, WindowManager::kFullScreen);

    // Create a toolbar for setting presets
    //
    toolBar = makeToolBar("Presets", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    action = app->getToolsMenuAction(App::kShowPresetsWindow);
    action->setIcon(QIcon(":/presetsWindow.png"));
    toolBar->addAction(action);
    toolBar->addWidget(new PresetChooser(configuration_, toolBar));

    // Create a toolbar for the colorbar
    //
    toolBar = makeToolBar("ColorMap", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    ColorMapWidget* colorMapWidget =
	new ColorMapWidget(configuration_->getVideoSampleCountTransform(),
                           toolBar);
    toolBar->addWidget(colorMapWidget);
    VideoImaging* videoImaging = configuration_->getVideoImaging();
    colorMapWidget->setColorMap(videoImaging->getColorMap());
    connect(videoImaging, SIGNAL(colorMapChanged(const QImage&)),
            colorMapWidget, SLOT(setColorMap(const QImage&)));
    connect(colorMapWidget, SIGNAL(changeColorMapType(int)),
            videoImaging, SLOT(setColorMapType(int)));

    toolBar = makeToolBar("View Chooser", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    action = app->getToolsMenuAction(App::kShowViewEditor);
    action->setIcon(QIcon(":/viewEditor.svg"));
    toolBar->addAction(action);
    ViewChooser* chooser = new ViewChooser(app->getViewEditor(), toolBar);
    chooser->setFocusPolicy(Qt::NoFocus);
    toolBar->addWidget(chooser);

    // Create a toolbar to show the plot symbol assignments.
    //
    toolBar = makeToolBar("Plot Symbols", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    TargetPlotSymbolsWidget* symbolsWidget =
	new TargetPlotSymbolsWidget(toolBar);
    toolBar->addWidget(symbolsWidget);
    symbolsWidget->connectExtractionsSymbolType(
	configuration_->getExtractionsImaging());
    symbolsWidget->connectRangeTruthsSymbolType(
	configuration_->getRangeTruthsImaging());
    symbolsWidget->connectBugPlotsSymbolType(
	configuration_->getBugPlotsImaging());

#if 0

    // Create a toolbar for the gain and min/max controls
    //
    toolBar = makeToolBar("Gain and Min/Max", Qt::RightToolBarArea);
    ControlsWidget* controlsWidget = new ControlsWidget(toolBar);
    controlsWidget->addControl("Gain", configuration_->getGainSetting());
    controlsWidget->addControl("Min", configuration_->getCutoffMinSetting());
    controlsWidget->addControl("Max", configuration_->getCutoffMaxSetting());
    toolBar->addWidget(controlsWidget);
#endif
}

void
MainWindow::on_actionViewClearAll__triggered()
{
    configuration_->getHistory()->clearAll();
    alphaBetaView_->clear();
    alphaRangeView_->clear();
}

void
MainWindow::on_actionViewClearVideo__triggered()
{
    configuration_->getHistory()->clearVideo();
    alphaBetaView_->clear();
    alphaRangeView_->clear();
}

void
MainWindow::on_actionViewClearExtractions__triggered()
{
    configuration_->getHistory()->clearExtractions();
}

void
MainWindow::on_actionViewClearRangeTruths__triggered()
{
    configuration_->getHistory()->clearRangeTruths();
}

void
MainWindow::on_actionViewClearBugPlots__triggered()
{
    configuration_->getHistory()->clearBugPlots();
}

void
MainWindow::on_actionPresetRevert__triggered()
{
    configuration_->restorePreset(configuration_->getActivePresetIndex());
    statusBar()->showMessage("Restored current preset.", 5000);
}

void
MainWindow::on_actionPresetSave__triggered()
{
    configuration_->savePreset(configuration_->getActivePresetIndex());
    statusBar()->showMessage("Saved current preset.", 5000);
}

void
MainWindow::updatePresetActions()
{
    bool dirty = configuration_->getActiveIsDirty();
    actionPresetSave_->setEnabled(dirty);
    actionPresetRevert_->setEnabled(dirty);
}

void
MainWindow::timerEvent(QTimerEvent* event)
{
    if (event->timerId() != updateTimer_.timerId()) {
	event->ignore();
	Super::timerEvent(event);
	return;
    }

    alphaBetaView_->refresh();
    alphaRangeView_->refresh();
}

void
MainWindow::showEvent(QShowEvent* event)
{
    Logger::ProcLog log("showEvent", Log());
    LOGINFO << std::endl;
    if (! updateTimer_.isActive())
	updateTimer_.start(kUpdateRate, this);
    Super::showEvent(event);
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

    if (updateTimer_.isActive())
	updateTimer_.stop();

    Super::closeEvent(event);
}

void
MainWindow::bugged()
{
    QPointF alphaBeta = alphaBetaView_->getDisplay()->getPendingBugPlot();
    if (alphaBeta.isNull()) return;
    QPointF alphaRange = alphaRangeView_->getDisplay()->getPendingBugPlot();
    if (alphaRange.isNull()) return;

    alphaBetaView_->getDisplay()->clearPendingBugPlot();
    alphaRangeView_->getDisplay()->clearPendingBugPlot();

    RadarSettings* radarSettings = configuration_->getRadarSettings();

    double range = alphaRange.y();
    double azimuth;
    double elevation;
    radarSettings->getAzimuthElevation(alphaBeta.x(), alphaBeta.y(),
                                       &azimuth, &elevation);
    BugPlotEmitterSettings* bpes = configuration_->getBugPlotEmitterSettings();
    Messages::BugPlot::Ref msg = bpes->addBugPlot(range, azimuth, elevation);
    if (msg) {
	configuration_->getHistory()->addBugPlot(msg);
    }
}

void
MainWindow::on_actionAlphaBetaViewFull__triggered()
{
    alphaBetaViewSettings_->popAll();
}

void
MainWindow::on_actionAlphaBetaViewPrevious__triggered()
{
    alphaBetaViewSettings_->pop();
}

void
MainWindow::on_actionAlphaBetaViewSwap__triggered()
{
    alphaBetaViewSettings_->swap();
}

void
MainWindow::updateAlphaBetaViewActions()
{
    bool canPop = alphaBetaViewSettings_->canPop(); 
    actionAlphaBetaViewFull_->setEnabled(canPop);
    actionAlphaBetaViewPrevious_->setEnabled(canPop);
    actionAlphaBetaViewSwap_->setEnabled(canPop);
}

void
MainWindow::on_actionAlphaRangeViewFull__triggered()
{
    alphaRangeViewSettings_->popAll();
}

void
MainWindow::on_actionAlphaRangeViewPrevious__triggered()
{
    alphaRangeViewSettings_->pop();
}

void
MainWindow::on_actionAlphaRangeViewSwap__triggered()
{
    alphaRangeViewSettings_->swap();
}

void
MainWindow::updateAlphaRangeViewActions()
{
    bool canPop = alphaRangeViewSettings_->canPop();
    actionAlphaRangeViewFull_->setEnabled(canPop);
    actionAlphaRangeViewPrevious_->setEnabled(canPop);
    actionAlphaRangeViewSwap_->setEnabled(canPop);
}
