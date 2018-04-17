#include "QtGui/QCloseEvent"
#include "QtGui/QIcon"
#include "QtWidgets/QMenu"
#include "QtWidgets/QMessageBox"
#include "QtWidgets/QStatusBar"
#include "QtWidgets/QToolButton"

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
#include "GUI/RangeTruthsImaging.h"
#include "GUI/SvgIconMaker.h"
#include "GUI/TargetPlotImaging.h"
#include "GUI/TargetPlotSymbolsWidget.h"
#include "GUI/ToolBar.h"
#include "GUI/Utils.h"
#include "GUI/WindowManager.h"
#include "GUI/Writers.h"

#include "BackgroundImageSettings.h"
#include "Configuration.h"
#include "CursorWidget.h"
#include "HistorySettings.h"
#include "HistoryWidget.h"
#include "MainWindow.h"
#include "VideoImaging.h"
#include "ViewSettings.h"

using namespace SideCar;
using namespace SideCar::GUI;
using namespace SideCar::GUI::PPIDisplay;

Logger::Log&
MainWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ppidisplay.MainWindow");
    return log_;
}

MainWindow::MainWindow() : MainWindowBase(), Ui::MainWindow(), writer_(0)
{
    Logger::ProcLog log("MainWindow", Log());
    LOGINFO << std::endl;

    setupUi(this);
#ifdef __DEBUG__
    setWindowTitle("PPIDisplay (DEBUG)");
#else
    setWindowTitle("PPIDisplay");
#endif

    App* app = getApp();
    Configuration* cfg = app->getConfiguration();
    updatePresetActions();
    connect(cfg, SIGNAL(presetDirtyStateChanged(int, bool)), SLOT(updatePresetActions()));
    connect(cfg, SIGNAL(activePresetChanged(int)), SLOT(updatePresetActions()));

    // Associate QAction objects with configuration settings.
    //
    cfg->getVideoImaging()->setToggleEnabledAction(actionViewToggleVideo_);
    cfg->getBinaryImaging()->setToggleEnabledAction(actionViewToggleBinary_);
    cfg->getExtractionsImaging()->setToggleEnabledAction(actionViewToggleExtractions_);
    cfg->getRangeTruthsImaging()->setToggleEnabledAction(actionViewToggleRangeTruths_);
    cfg->getBugPlotsImaging()->setToggleEnabledAction(actionViewToggleBugPlots_);
    cfg->getRangeMapImaging()->setToggleEnabledAction(actionViewToggleRangeMap_);
    cfg->getRangeRingsImaging()->setToggleEnabledAction(actionViewToggleRangeRings_);
    cfg->getBackgroundImageSettings()->setToggleEnabledAction(actionViewToggleBackground_);
    cfg->getShowCursorPositionSetting()->setToggleEnabledAction(actionViewToggleShowCursorPosition_);
    cfg->getShowPhantomCursorSetting()->setToggleEnabledAction(actionViewTogglePhantomCursor_);

    connect(cfg->getPhantomCursorImaging(), SIGNAL(enabledChanged(bool)), actionViewTogglePhantomCursor_,
            SLOT(setEnabled(bool)));
    actionViewTogglePhantomCursor_->setEnabled(cfg->getPhantomCursorImaging()->isEnabled());

    connect(cfg->getShowCursorPositionSetting(), SIGNAL(enabledChanged(bool)), display_,
            SLOT(setShowCursorPosition(bool)));
    display_->setShowCursorPosition(cfg->getShowCursorPositionSetting()->isEnabled());

    // Associate icons with QAction objects
    //
    actionViewReset_->setIcon(QIcon(":/home.png"));
    actionViewClearAll_->setIcon(QIcon(":/refresh.png"));
    actionViewZoomIn_->setIcon(QIcon(":/zoomIn.png"));
    actionViewZoomOut_->setIcon(QIcon(":/zoomOut.png"));
    actionViewPanLeft_->setIcon(QIcon(":/leftArrow.png"));
    actionViewPanRight_->setIcon(QIcon(":/rightArrow.png"));
    actionViewPanUp_->setIcon(QIcon(":/upArrow.png"));
    actionViewPanDown_->setIcon(QIcon(":/downArrow.png"));

    // For these QAction objects, set shortcut values that could not be entered via the Qt Designer application.
    //
    actionViewPanLeft_->setShortcut(Qt::Key_Left);
    actionViewPanRight_->setShortcut(Qt::Key_Right);
    actionViewPanUp_->setShortcut(Qt::Key_Up);
    actionViewPanDown_->setShortcut(Qt::Key_Down);

    // Associate icons with QAction objects that toggle data display
    //
    SvgIconMaker im;
    actionViewToggleVideo_->setIcon(im.make('V'));
    actionViewToggleBinary_->setIcon(im.make('B'));
    actionViewToggleExtractions_->setIcon(im.make('E'));
    actionViewToggleRangeTruths_->setIcon(im.make('T'));
    actionViewToggleBugPlots_->setIcon(im.make('P'));
    actionViewToggleRangeMap_->setIcon(im.make('M'));
    actionViewToggleRangeRings_->setIcon(im.make('R'));
    actionViewToggleBackground_->setIcon(im.make('G'));
    actionViewToggleShowCursorPosition_->setIcon(im.make("cursorPosition"));
    actionViewTogglePhantomCursor_->setIcon(im.make("phantomCursor"));

    ToolBar* toolBar;
    QAction* action;

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
    QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(actionViewToggleVideo_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(new ChannelMenu(cfg->getVideoChannel(), toolButton));

    toolBar->addAction(actionViewToggleBinary_);
    toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(actionViewToggleBinary_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(new ChannelMenu(cfg->getBinaryChannel(), toolButton));

    toolBar->addAction(actionViewToggleExtractions_);
    toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(actionViewToggleExtractions_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(new ChannelMenu(cfg->getExtractionsChannel(), toolButton));

    toolBar->addAction(actionViewToggleRangeTruths_);
    toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(actionViewToggleRangeTruths_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(new ChannelMenu(cfg->getRangeTruthsChannel(), toolButton));

    toolBar->addAction(actionViewToggleBugPlots_);
    toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(actionViewToggleBugPlots_));
    toolButton->setPopupMode(QToolButton::DelayedPopup);
    toolButton->setMenu(new ChannelMenu(cfg->getBugPlotsChannel(), toolButton));

    toolBar->addAction(actionViewToggleRangeMap_);
    toolBar->addAction(actionViewToggleRangeRings_);
    toolBar->addAction(actionViewToggleBackground_);
    toolBar->addAction(actionViewToggleShowCursorPosition_);
    toolBar->addAction(actionViewTogglePhantomCursor_);

    // Create a toolbar for the QAction objects that modify the view transformation (zooming and panning).
    //
    toolBar = makeToolBar("View Actions", Qt::LeftToolBarArea);
    toolBar->addAction(actionViewZoomIn_);
    toolBar->addAction(actionViewZoomOut_);
    toolBar->addAction(actionViewPanLeft_);
    toolBar->addAction(actionViewPanRight_);
    toolBar->addAction(actionViewPanUp_);
    toolBar->addAction(actionViewPanDown_);
    toolBar->addAction(actionViewReset_);
    toolBar->addAction(actionViewClearAll_);

    app->addWindowMenuActionTo(toolBar, WindowManager::kFullScreen);

    // Cursor info widget
    //
    CursorWidget* cursorWidget = new CursorWidget(statusBar());
    statusBar()->addPermanentWidget(cursorWidget);
    connect(app, SIGNAL(phantomCursorChanged(const QPointF&)), cursorWidget, SLOT(showCursorPosition(const QPointF&)));

    // Radar info widget
    //
    RadarInfoWidget* radarInfoWidget = new RadarInfoWidget(statusBar());
    statusBar()->addPermanentWidget(radarInfoWidget);
    connect(display_, SIGNAL(currentMessage(Messages::PRIMessage::Ref)), radarInfoWidget,
            SLOT(showMessageInfo(Messages::PRIMessage::Ref)));

    // Channel connection info widget
    //
    ChannelInfoWidget* channelInfoWidget = new ChannelInfoWidget(statusBar());
    statusBar()->addPermanentWidget(channelInfoWidget);
    channelInfoWidget->addChannel("V:", cfg->getVideoChannel());
    channelInfoWidget->addChannel("B:", cfg->getBinaryChannel());
    channelInfoWidget->addChannel("E:", cfg->getExtractionsChannel());
    channelInfoWidget->addChannel("T:", cfg->getRangeTruthsChannel());
    channelInfoWidget->addChannel("P:", cfg->getBugPlotsChannel());

    // Create a toolbar for setting presets
    //
    toolBar = makeToolBar("Presets", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    action = app->getToolsMenuAction(App::kShowPresetsWindow);
    action->setIcon(QIcon(":/viewEditor.svg"));
    toolBar->addAction(action);

    PresetChooser* presetChooser = new PresetChooser(cfg, toolBar);
    toolBar->addWidget(presetChooser);
    connect(cfg, SIGNAL(presetDirtyStateChanged(int, bool)), SLOT(monitorPresets(int, bool)));

    // Create a toolbar to display the colormap
    //
    toolBar = makeToolBar("Video ColorMap", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    ColorMapWidget* colorMapWidget = new ColorMapWidget(cfg->getVideoSampleCountTransform(), toolBar);
    toolBar->addWidget(colorMapWidget);
    VideoImaging* videoImaging = cfg->getVideoImaging();
    colorMapWidget->setColorMap(videoImaging->getColorMap());
    connect(videoImaging, SIGNAL(colorMapChanged(const QImage&)), colorMapWidget, SLOT(setColorMap(const QImage&)));
    connect(colorMapWidget, SIGNAL(changeColorMapType(int)), videoImaging, SLOT(setColorMapType(int)));

    // Create a toolbar for the history control
    //
    toolBar = makeToolBar("History", Qt::TopToolBarArea);
    HistoryWidget* historyWidget = new HistoryWidget(app->getHistory(), toolBar);
    toolBar->addWidget(historyWidget);
    toolBar->toggleVisibility();

    // Create a toolbar to show the plot symbol assignments.
    //
    toolBar = makeToolBar("Plot Symbols", Qt::TopToolBarArea);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    TargetPlotSymbolsWidget* symbolsWidget = new TargetPlotSymbolsWidget(toolBar);
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

    connect(cfg->getShowPhantomCursorSetting(), SIGNAL(enabledChanged(bool)), display_, SLOT(showPhantomCursor(bool)));
    display_->showPhantomCursor(cfg->getShowPhantomCursorSetting()->isEnabled());

    setAttribute(Qt::WA_TranslucentBackground, false);
    setWindowOpacity(1.0);
}

void
MainWindow::showEvent(QShowEvent* event)
{
    Super::showEvent(event);
    QTimer::singleShot(0, display_, SLOT(raiseMagnifiers()));
    display_->setFocus();
}

void
MainWindow::closeEvent(QCloseEvent* event)
{
    App* app = getApp();

    // Since we are the only window that should be alive, tell the application to quit. Only do this if the
    // application is not already in the process of shutting down.
    //
    if (!app->isQuitting()) {
        event->ignore();
        QTimer::singleShot(0, app, SLOT(applicationQuit()));
        return;
    }

    Super::closeEvent(event);
}

void
MainWindow::on_actionViewZoomIn__triggered()
{
    display_->changeZoom(1);
}

void
MainWindow::on_actionViewZoomOut__triggered()
{
    display_->changeZoom(-1);
}

void
MainWindow::on_actionViewPanLeft__triggered()
{
    display_->pan(-0.5, 0.0);
}

void
MainWindow::on_actionViewPanRight__triggered()
{
    display_->pan(0.5, 0.0);
}

void
MainWindow::on_actionViewPanUp__triggered()
{
    display_->pan(0.0, 0.5);
}

void
MainWindow::on_actionViewPanDown__triggered()
{
    display_->pan(0.0, -0.5);
}

void
MainWindow::on_actionViewReset__triggered()
{
    getApp()->getConfiguration()->getViewSettings()->reset();
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
MainWindow::on_actionViewCenterAtCursor__triggered()
{
    display_->centerAtCursor();
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
    if (!isDirty)
        statusBar()->showMessage("Preset saved.", 5000);
    else
        statusBar()->showMessage("Preset changed.", 5000);
}
